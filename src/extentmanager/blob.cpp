/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "blob.h"

#include "common/config.h"
#include "common/error_code.h"
#include "utils/chrono.h"
#include "utils/pb_transfer.h"

namespace cyprestore {
namespace extentmanager {

bool blobsize_valid(uint64_t size) {
    return size > 0 && size <= common::kMaxBlobSize
           && (size % GlobalConfig().extentmanager().extent_size == 0);
}

Blob::Blob(
        const std::string &id, const std::string &name, uint64_t size,
        BlobType type, const std::string &desc, const std::string &pool_id,
        const std::string &user_id, const std::string &instance_id)
        : id_(id), name_(name), size_(size), type_(type), desc_(desc),
          status_(kBlobStatusCreated), pool_id_(pool_id), user_id_(user_id),
          instance_id_(instance_id) {
    create_time_ = utils::Chrono::DateString();
    update_time_ = create_time_;
}

common::pb::Blob Blob::ToPbBlob() {
    common::pb::Blob pb_blob;
    pb_blob.set_id(id_);
    pb_blob.set_name(name_);
    pb_blob.set_size(size_);
    pb_blob.set_type(utils::toPbBlobType(type_));
    pb_blob.set_desc(desc_);
    pb_blob.set_status(utils::toPbBlobStatus(status_));
    pb_blob.set_pool_id(pool_id_);
    pb_blob.set_user_id(user_id_);
    pb_blob.set_instance_id(instance_id_);
    pb_blob.set_create_date(create_time_);
    pb_blob.set_update_date(update_time_);

    return pb_blob;
}

Status BlobManager::create_blob(
        const std::string &id, const std::string &name, uint64_t size,
        BlobType type, const std::string &desc, const std::string &pool_id,
        const std::string &user_id, const std::string &instance_id) {
    // blob name not need unique, so no need check
    std::unique_ptr<Blob> blob(new Blob(
            id, name, size, type, desc, pool_id, user_id, instance_id));

    std::string value = utils::Serializer<Blob>::Encode(*blob.get());
    auto status = kv_store_->Put(blob->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "Persist blob meta to store failed, status: "
                   << status.ToString() << ", blob_id: " << id;
        return Status(
                common::CYPRE_EM_STORE_ERROR,
                "persist blob meta to store failed");
    }

    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    blob_map_[blob->id_] = std::move(blob);
    return Status();
}

Status BlobManager::soft_delete_blob(
        const std::string &user_id, const std::string &blob_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = blob_map_.find(blob_id);
    if (iter == blob_map_.end()
        || iter->second->status_ == kBlobStatusDeleted) {
        LOG(ERROR) << "Not find blob, blob id: " << blob_id;
        return Status(common::CYPRE_EM_BLOB_NOT_FOUND, "not find blob");
    }
    if (iter->second->user_id_ != user_id) {
        LOG(ERROR) << "Have no permisson to delete blob"
                   << ", expected user: " << iter->second->user_id_
                   << ", actual user: " << user_id;
        return Status(common::CYPRE_ER_NO_PERMISSION, "userid not correct");
    }

    iter->second->status_ = kBlobStatusDeleted;
    std::string value = utils::Serializer<Blob>::Encode(*iter->second.get());
    auto status = kv_store_->Put(iter->second->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "Delete blob meta from store failed, "
                   << status.ToString() << ", blob id: " << blob_id;
        return Status(
                common::CYPRE_EM_STORE_ERROR,
                "delete blob meta from store failed");
    }
    return Status();
}

Status BlobManager::rename_blob(
        const std::string &user_id, const std::string &blob_id,
        const std::string &new_name) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = blob_map_.find(blob_id);
    if (iter == blob_map_.end()
        || iter->second->status_ == kBlobStatusDeleted) {
        LOG(ERROR) << "Not find blob, blob_id: " << blob_id;
        return Status(common::CYPRE_EM_BLOB_NOT_FOUND, "not found blob");
    }
    if (iter->second->user_id_ != user_id) {
        LOG(ERROR) << "Have no permisson to rename blob"
                   << ", expected user: " << iter->second->user_id_
                   << ", actual user: " << user_id;
        return Status(common::CYPRE_ER_NO_PERMISSION, "userid not correct");
    }

    std::string old_name = iter->second->name_;
    std::string old_time = iter->second->update_time_;
    iter->second->name_ = new_name;
    iter->second->update_time_ = utils::Chrono::DateString();

    std::string value = utils::Serializer<Blob>::Encode(*(iter->second.get()));
    auto status = kv_store_->Put(iter->second->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "Persist blob meta to store failed, status: "
                   << status.ToString() << ", blob_id: " << blob_id;
        iter->second->name_ = old_name;
        iter->second->update_time_ = old_time;
        return Status(
                common::CYPRE_EM_STORE_ERROR,
                "persist blob meta to store failed");
    }
    LOG(INFO) << "Rename blob name succeed, blob id: " << blob_id
              << ", old name: " << old_name << ", new name: " << new_name;
    return Status();
}

Status BlobManager::resize_blob(
        const std::string &user_id, const std::string &blob_id,
        uint64_t new_size, uint64_t *old_size) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = blob_map_.find(blob_id);
    if (iter == blob_map_.end()
        || iter->second->status_ == kBlobStatusDeleted) {
        LOG(ERROR) << "Not find blob, blob_id: " << blob_id;
        return Status(common::CYPRE_EM_BLOB_NOT_FOUND, "not find blob");
    }
    if (iter->second->user_id_ != user_id) {
        LOG(ERROR) << "Have no permisson to resize blob"
                   << ", expected user: " << iter->second->user_id_
                   << ", actual user: " << user_id;
        return Status(common::CYPRE_ER_NO_PERMISSION, "userid not correct");
    }

    auto size = iter->second->size_;
    if (!blobsize_valid(new_size) || new_size <= size) {
        LOG(ERROR) << "Resize blob failed, new size invalid or is smaller than "
                      "old size"
                   << ", new size: " << new_size << ", old size: " << size;
        return Status(
                common::CYPRE_ER_INVALID_ARGUMENT,
                "new size invalid or smaller than old size");
    }
    *old_size = size;

    std::string old_time = iter->second->update_time_;
    iter->second->size_ = new_size;
    iter->second->update_time_ = utils::Chrono::DateString();

    std::string value = utils::Serializer<Blob>::Encode(*(iter->second.get()));
    auto status = kv_store_->Put(iter->second->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "Persist blob meta to store failed, status: "
                   << status.ToString() << ", blob_id: " << blob_id;
        iter->second->size_ = size;
        iter->second->update_time_ = old_time;
        return Status(
                common::CYPRE_EM_STORE_ERROR,
                "persist blob meta to store failed");
    }

    LOG(INFO) << "Resize blob succeed, blob id: " << blob_id
              << ", old size: " << size << ", new size: " << new_size;
    return Status();
}

// only called when process first start
bool BlobManager::recovery_from_store(const std::string &pool_id) {
    std::unique_ptr<kvstore::KVIterator> iter;
    std::stringstream ss(
            common::kBlobKvPrefix, std::ios_base::app | std::ios_base::out);
    ss << "_" << pool_id;
    std::string prefix = ss.str();
    auto status = kv_store_->ScanPrefix(prefix, &iter);
    if (!status.ok()) {
        LOG(ERROR) << "Scan blob meta from store failed, status: "
                   << status.ToString();
        return false;
    }

    std::vector<std::string> tmp;
    while (iter->Valid()) {
        tmp.push_back(iter->value());
        iter->Next();
    }
    for (auto &v : tmp) {
        std::unique_ptr<Blob> blob(new Blob());
        if (utils::Serializer<Blob>::Decode(v, *blob.get())) {
            blob_map_[blob->id_] = std::move(blob);
        } else {
            LOG(ERROR) << "Decode blob meta error";
            return false;
        }
    }

    LOG(INFO) << "Recovery blob meta from store success, include "
              << blob_map_.size() << " blobs.";
    return true;
}

Status BlobManager::query_blob(
        const std::string &blob_id, bool all, common::pb::Blob *blob) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = blob_map_.find(blob_id);
    if (iter == blob_map_.end()
        || (!all && iter->second->status_ == kBlobStatusDeleted)) {
        LOG(ERROR) << "Not find blob, blob_id: " << blob_id;
        return Status(common::CYPRE_EM_BLOB_NOT_FOUND, "not find blob");
    }
    *blob = iter->second->ToPbBlob();
    return Status();
}

Status BlobManager::list_blobs(
        const std::string &user_id, std::vector<common::pb::Blob> *blobs) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &b : blob_map_) {
        if (b.second->user_id_ == user_id
            && b.second->status_ != kBlobStatusDeleted) {
            blobs->push_back(b.second->ToPbBlob());
        }
    }
    return Status();
}

Status BlobManager::list_blobs(
        const std::string &user_id, std::vector<std::string> *blobs) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &b : blob_map_) {
        if (b.second->user_id_ == user_id) {
            blobs->push_back(b.first);
        }
    }
    return Status();
}

Status BlobManager::list_blobs(bool deleted, std::vector<std::string> *blobs) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &b : blob_map_) {
        if (deleted && b.second->status_ == kBlobStatusDeleted) {
            blobs->push_back(b.first);
        } else if (!deleted) {
            blobs->push_back(b.first);
        }
    }
    return Status();
}

Status BlobManager::delete_blob(const std::string &blob_id) {
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        auto iter = blob_map_.find(blob_id);
        if (iter == blob_map_.end()) {
            return Status(common::CYPRE_EM_BLOB_NOT_FOUND, "not found blob");
        }
        auto status = kv_store_->Delete(iter->second->kv_key());
        if (!status.ok()) {
            LOG(ERROR) << "Delete blob meta from store failed, status: "
                       << status.ToString() << ", blob_id: " << blob_id;
            return Status(
                    common::CYPRE_EM_DELETE_ERROR,
                    "Delete blob meta from store failed");
        }
    }
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    blob_map_.erase(blob_id);
    return Status();
}

bool BlobManager::blobid_exist(const std::string &blob_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    return blob_map_.find(blob_id) != blob_map_.end();
}

bool BlobManager::blob_valid(const std::string &blob_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    auto iter = blob_map_.find(blob_id);

    return iter != blob_map_.end()
           && iter->second->status_ != kBlobStatusDeleted;
}

}  // namespace extentmanager
}  // namespace cyprestore
