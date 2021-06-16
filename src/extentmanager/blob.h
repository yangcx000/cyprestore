/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTMANAGER_BLOB_H_
#define CYPRESTORE_EXTENTMANAGER_BLOB_H_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cereal/access.hpp>
#include <cereal/types/string.hpp>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/constants.h"
#include "common/pb/types.pb.h"
#include "common/status.h"
#include "enum.h"
#include "kvstore/rocks_store.h"
#include "utils/serializer.h"

namespace cyprestore {
namespace extentmanager {

bool blobsize_valid(uint64_t size);

class Blob {
public:
    Blob() = default;
    Blob(const std::string &id, const std::string &name, uint64_t size,
         BlobType type, const std::string &desc, const std::string &pool_id,
         const std::string &user_id, const std::string &instance_id);
    ~Blob() = default;

    std::string kv_key() {
        std::stringstream ss(
                common::kBlobKvPrefix, std::ios_base::app | std::ios_base::out);
        ss << "_" << pool_id_ << "_" << id_;
        return ss.str();
    }
    common::pb::Blob ToPbBlob();

private:
    friend class BlobManager;
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive &archive, const std::uint32_t version) {
        archive(id_);
        archive(name_);
        archive(size_);
        archive(type_);
        archive(desc_);
        archive(status_);
        archive(pool_id_);
        archive(user_id_);
        archive(instance_id_);
        archive(create_time_);
        archive(update_time_);
    }

    std::string id_;
    std::string name_;
    uint64_t size_;
    BlobType type_;
    std::string desc_;
    BlobStatus status_;
    std::string pool_id_;
    std::string user_id_;
    // set id ?
    std::string instance_id_;
    std::string create_time_;
    std::string update_time_;
};

using common::Status;

class BlobManager {
public:
    BlobManager() = default;
    BlobManager(std::shared_ptr<kvstore::RocksStore> kv_store) {
        kv_store_ = kv_store;
    }
    ~BlobManager() = default;

    Status create_blob(
            const std::string &id, const std::string &name, uint64_t size,
            BlobType type, const std::string &desc, const std::string &pool_id,
            const std::string &user_id, const std::string &instance_id);
    Status
    soft_delete_blob(const std::string &user_id, const std::string &blob_id);
    Status delete_blob(const std::string &blob_id);
    Status rename_blob(
            const std::string &user_id, const std::string &blob_id,
            const std::string &new_name);
    Status resize_blob(
            const std::string &user_id, const std::string &blob_id,
            uint64_t new_size, uint64_t *old_size);
    Status
    query_blob(const std::string &blob_id, bool all, common::pb::Blob *blob);
    Status list_blobs(
            const std::string &user_id, std::vector<common::pb::Blob> *blobs);
    Status
    list_blobs(const std::string &user_id, std::vector<std::string> *blobs);
    Status list_blobs(bool deleted, std::vector<std::string> *blobs);
    bool blobid_exist(const std::string &blob_id);
    bool blob_valid(const std::string &blob_id);
    bool recovery_from_store(const std::string &pool_id);

private:
    // blob_id -> blob
    std::unordered_map<std::string, std::unique_ptr<Blob>> blob_map_;
    boost::shared_mutex mutex_;
    std::shared_ptr<kvstore::RocksStore> kv_store_;
};

}  // namespace extentmanager
}  // namespace cyprestore

// this macro must be placed at global scope
CEREAL_CLASS_VERSION(cyprestore::extentmanager::Blob, 0);

#endif  // CYPRESTORE_EXTENTMANAGER_BLOB_H_
