/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "user.h"

#include <algorithm>

#include "butil/logging.h"
#include "common/constants.h"
#include "common/error_code.h"
#include "utils/chrono.h"
#include "utils/pb_transfer.h"
#include "utils/uuid.h"

namespace cyprestore {
namespace extentmanager {

User::User(
        const std::string &id, const std::string &name,
        const std::string &email, const std::string &comments, int set_id)
        : id_(id), name_(name), email_(email), comments_(comments),
          status_(kUserStatusCreated), set_id_(set_id) {
    create_time_ = utils::Chrono::DateString();
    update_time_ = create_time_;
}

std::string User::get_poolid() {
    std::stringstream ss(std::ios_base::app | std::ios_base::out);
    for (auto &v : pool_ids_) {
        ss << v << ", ";
    }
    return ss.str();
}

common::pb::User User::ToPbUser() {
    common::pb::User pb_user;
    pb_user.set_id(id_);
    pb_user.set_name(name_);
    pb_user.set_email(email_);
    pb_user.set_comments(comments_);
    pb_user.set_status(utils::ToPbUserStatus(status_));
    pb_user.set_pool_id(get_poolid());
    pb_user.set_set_id(set_id_);
    pb_user.set_create_date(create_time_);
    pb_user.set_update_date(update_time_);

    return pb_user;
}

Status UserManager::create_user(
        const std::string &name, const std::string &email,
        const std::string &comments, int set_id, std::string *user_id) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    if (username_exist(name)) {
        LOG(ERROR) << "create user failed, duplicated user name: " << name;
        return Status(common::CYPRE_EM_USER_DUPLICATED, "duplicated user name");
    }
    std::string id = gen_userid();
    std::unique_ptr<User> user(new User(id, name, email, comments, set_id));

    // persist
    std::string value = utils::Serializer<User>::Encode(*user.get());
    auto status = kv_store_->Put(user->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "Persist user meta to store failed, status: "
                   << status.ToString() << ", user_id: " << id
                   << ", user_name: " << name;
        return Status(common::CYPRE_EM_STORE_ERROR, "persist user meta failed");
    }

    *user_id = user->id_;
    user_map_[user->id_] = std::move(user);
    LOG(INFO) << "Create user success"
              << ", user name: " << name << ", user id: " << id
              << ", email: " << email << ", set_id: " << set_id;
    return Status();
}

std::string UserManager::gen_userid() {
    std::string user_id;
    do {
        user_id = common::kUserIdPrefix + utils::UUID::Generate();
        if (user_map_.find(user_id) == user_map_.end()) {
            return user_id;
        }
    } while (true);
}

bool UserManager::username_exist(const std::string &user_name) {
    for (auto &u : user_map_) {
        if (u.second->status_ == kUserStatusCreated
            && u.second->name_ == user_name) {
            return true;
        }
    }
    return false;
}

Status UserManager::update_user(
        const std::string &user_id, const std::string &new_name,
        const std::string &new_email, const std::string &new_comments) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = user_map_.find(user_id);
    if (iter == user_map_.end()
        || iter->second->status_ == kUserStatusDeleted) {
        LOG(ERROR) << "Rename user name failed, not find user id: " << user_id;
        return Status(common::CYPRE_EM_USER_NOT_FOUND, "not found user");
    }

    // update name
    std::string old_name = iter->second->name_;
    if (username_exist(new_name)) {
        LOG(ERROR) << "Rename user name failed, duplicated user name: "
                   << new_name;
        return Status(common::CYPRE_EM_USER_DUPLICATED, "duplicated user name");
    }
    iter->second->name_ = new_name.empty() ? old_name : new_name;

    // update email
    std::string old_email = iter->second->email_;
    iter->second->email_ = new_email.empty() ? old_email : new_email;

    // update comments
    std::string old_comments = iter->second->comments_;
    iter->second->comments_ =
            new_comments.empty() ? old_comments : new_comments;

    // update update_time
    std::string old_time = iter->second->update_time_;
    iter->second->update_time_ = utils::Chrono::DateString();

    // persist
    std::string value = utils::Serializer<User>::Encode(*(iter->second.get()));
    auto status = kv_store_->Put(iter->second->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "Persist user meta to store failed, status: "
                   << status.ToString() << ", user_id: " << user_id
                   << "old name: " << old_name;
        // persist faild, recover old information
        iter->second->name_ = old_name;
        iter->second->email_ = old_email;
        iter->second->comments_ = old_comments;
        iter->second->update_time_ = old_time;
        return Status(common::CYPRE_EM_STORE_ERROR, "persist user meta failed");
    }
    LOG(INFO) << "Update user succeed, user id: " << user_id;
    return Status();
}

Status UserManager::soft_delete_user(const std::string &user_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = user_map_.find(user_id);
    if (iter == user_map_.end()) {
        LOG(ERROR) << "Not find user, id: " << user_id;
        return Status(common::CYPRE_EM_USER_NOT_FOUND, "not found user");
    }

    iter->second->status_ = kUserStatusDeleted;
    std::string value = utils::Serializer<User>::Encode(*(iter->second.get()));
    auto status = kv_store_->Put(iter->second->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "Delete user meta from store failed, status: "
                   << status.ToString() << ", user_id: " << user_id;
        return Status(
                common::CYPRE_EM_STORE_ERROR,
                "delete user meta from store failed");
    }

    LOG(INFO) << "Delete user succeed, user_id: " << user_id;
    return Status();
}

Status UserManager::delete_user(const std::string &user_id) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    auto iter = user_map_.find(user_id);
    if (iter == user_map_.end()) {
        LOG(ERROR) << "Not find user, id: " << user_id;
        return Status(common::CYPRE_EM_USER_NOT_FOUND, "not found user");
    }

    auto status = kv_store_->Delete(iter->second->kv_key());
    if (!status.ok()) {
        LOG(ERROR) << "Delete user meta from store failed, status: "
                   << status.ToString() << ", user_id: " << user_id;
        return Status(
                common::CYPRE_EM_DELETE_ERROR,
                "delete user meta from store failed");
    }
    user_map_.erase(iter);
    LOG(INFO) << "Delete user succeed, user_id: " << user_id;
    return Status();
}

Status
UserManager::add_pool(const std::string &user_id, const std::string &pool_id) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    auto iter = user_map_.find(user_id);
    if (iter == user_map_.end()
        || iter->second->status_ == kUserStatusDeleted) {
        LOG(ERROR) << "Not find user, id: " << user_id;
        return Status(common::CYPRE_EM_USER_NOT_FOUND, "not found user");
    }
    std::vector<std::string>::iterator it =
            find(iter->second->pool_ids_.begin(), iter->second->pool_ids_.end(),
                 pool_id);
    if (it != iter->second->pool_ids_.end()) {
        return Status();
    }
    iter->second->pool_ids_.push_back(pool_id);
    return Status();
}

Status UserManager::list_pools(
        const std::string &user_id, std::vector<std::string> *pools) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = user_map_.find(user_id);
    if (iter == user_map_.end()) {
        LOG(ERROR) << "Not find user, id: " << user_id;
        return Status(common::CYPRE_EM_USER_NOT_FOUND, "not found user");
    }
    for (auto &v : iter->second->pool_ids_) {
        pools->push_back(v);
    }
    return Status();
}

Status
UserManager::query_user(const std::string &user_id, common::pb::User *user) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = user_map_.find(user_id);
    if (iter == user_map_.end()
        || iter->second->status_ == kUserStatusDeleted) {
        LOG(ERROR) << "Not find user, user id: " << user_id;
        return Status(common::CYPRE_EM_USER_NOT_FOUND, "not found user");
    }
    *user = iter->second->ToPbUser();
    return Status();
}

Status UserManager::list_users(std::vector<common::pb::User> *users) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &u : user_map_) {
        if (u.second->status_ == kUserStatusCreated) {
            users->push_back(u.second->ToPbUser());
        }
    }
    return Status();
}

Status UserManager::list_deleted_users(std::vector<std::string> *users) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &u : user_map_) {
        if (u.second->status_ == kUserStatusDeleted) {
            users->push_back(u.first);
        }
    }
    return Status();
}

bool UserManager::user_exist(const std::string &user_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    auto iter = user_map_.find(user_id);
    return iter != user_map_.end()
           && iter->second->status_ != kUserStatusDeleted;
}

// only called when process first start
bool UserManager::recovery_from_store() {
    // no need to lock
    std::unique_ptr<kvstore::KVIterator> iter;
    auto status =
            kv_store_->ScanPrefix(cyprestore::common::kUserKvPrefix, &iter);
    if (!status.ok()) {
        LOG(ERROR) << "Scan user meta from kv failed, status: "
                   << status.ToString();
        return false;
    }
    std::vector<std::string> tmp;
    while (iter->Valid()) {
        tmp.push_back(iter->value());
        iter->Next();
    }
    for (auto &v : tmp) {
        std::unique_ptr<User> user(new User());
        if (utils::Serializer<User>::Decode(v, *user.get())) {
            user_map_[user->id_] = std::move(user);
        } else {
            LOG(ERROR) << "Decode user meta failed.";
            return false;
        }
    }
    LOG(INFO) << "Recovery user meta from store success, include "
              << user_map_.size() << " users.";
    return true;
}

}  // namespace extentmanager
}  // namespace cyprestore
