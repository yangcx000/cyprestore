/*
 * Copyright 2020 JDD authors
 * @feifei5
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_USER_H_
#define CYPRESTORE_EXTENTMANAGER_USER_H_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cereal/access.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "common/constants.h"
#include "common/pb/types.pb.h"
#include "common/status.h"
#include "enum.h"
#include "kvstore/rocks_store.h"
#include "utils/serializer.h"

namespace cyprestore {
namespace extentmanager {

class User {
public:
    User() = default;
    User(const std::string &id, const std::string &name,
         const std::string &email, const std::string &comments, int set_id);
    ~User() = default;

    common::pb::User ToPbUser();
    std::string kv_key() {
        std::stringstream ss(
                common::kUserKvPrefix, std::ios_base::app | std::ios_base::out);
        ss << "_" << id_;
        return ss.str();
    }
    std::string get_poolid();

private:
    friend class UserManager;
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive &archive, const std::uint32_t version) {
        archive(id_);
        archive(name_);
        archive(email_);
        archive(comments_);
        archive(status_);
        archive(pool_ids_);
        archive(set_id_);
        archive(create_time_);
        archive(update_time_);
    }

    std::string id_;    // unique in zone and set
    std::string name_;  // unique in zone and set
    std::string email_;
    std::string comments_;
    UserStatus status_;
    // TODO should update when use a new pool
    std::vector<std::string> pool_ids_;
    int set_id_;
    std::string create_time_;
    std::string update_time_;
};

using common::Status;

class UserManager {
public:
    UserManager(std::shared_ptr<kvstore::RocksStore> kv_store)
            : kv_store_(kv_store) {}
    ~UserManager() = default;

    Status create_user(
            const std::string &name, const std::string &email,
            const std::string &comments, int set_id, std::string *user);
    Status update_user(
            const std::string &user_id, const std::string &new_name,
            const std::string &new_email, const std::string &new_comments);
    Status add_pool(const std::string &user_id, const std::string &pool_id);
    Status
    list_pools(const std::string &user_id, std::vector<std::string> *pools);
    Status soft_delete_user(const std::string &user_id);
    Status delete_user(const std::string &user_id);
    Status query_user(const std::string &usre_id, common::pb::User *user);
    Status list_users(std::vector<common::pb::User> *users);
    Status list_deleted_users(std::vector<std::string> *users);
    bool user_exist(const std::string &user_id);
    bool recovery_from_store();

private:
    std::string gen_userid();
    bool username_exist(const std::string &user_name);

    // userid -> User
    std::unordered_map<std::string, std::unique_ptr<User>> user_map_;
    boost::shared_mutex mutex_;

    std::shared_ptr<kvstore::RocksStore> kv_store_;
};

}  // namespace extentmanager
}  // namespace cyprestore

CEREAL_CLASS_VERSION(cyprestore::extentmanager::User, 0);

#endif  // CYPRESTORE_EXTENTMANAGER_USER_H_
