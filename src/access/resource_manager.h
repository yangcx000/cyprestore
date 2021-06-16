/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_ACCESS_RESOURCE_MANAGER_H
#define CYPRESTORE_ACCESS_RESOURCE_MANAGER_H

#include <brpc/channel.h>

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/pb/types.pb.h"
#include "common/status.h"

namespace cyprestore {
namespace access {

using common::pb::Status;

class ResourceMgr {
public:
    ResourceMgr() = default;
    ~ResourceMgr() = default;

    // user api
    Status CreateUser(
            const std::string &user_name, const std::string &email,
            const std::string &comments, int set_id, const std::string &pool_id,
            common::pb::PoolType type, std::string *user_id);
    Status DeleteUser(const std::string &user_id);
    Status UpdateUser(
            const std::string &user_id, const std::string &new_name,
            const std::string &new_email, const std::string &new_comments);
    Status QueryUser(const std::string &user_id, common::pb::User *userinfo);
    Status ListUsers(int set_id, std::vector<common::pb::User> *users);

    // blob api
    Status CreateBlob(
            const std::string &user_id, const std::string &blob_name,
            uint64_t blob_size, common::pb::BlobType type,
            const std::string &instance_id, const std::string &blob_desc,
            const std::string &pool_id, std::string *blob_id);
    Status DeleteBlob(const std::string &user_id, const std::string &blob_id);
    Status RenameBlob(
            const std::string &user_id, const std::string &blob_id,
            const std::string &new_name);
    Status ResizeBlob(
            const std::string &user_id, const std::string &blob_id,
            uint64_t blob_size);
    Status QueryBlob(
            const std::string &user_id, const std::string &blob_id,
            common::pb::Blob *blob);
    Status
    ListBlobs(const std::string &user_id, std::vector<common::pb::Blob> *blobs);

private:
    std::shared_ptr<brpc::Channel>
    GetChannel(const std::string &em_ip, int em_port);
    std::shared_ptr<brpc::Channel>
    InitChannel(const std::string &em_ip, int em_port);
    Status
    QueryUserSet(const std::string &user_id, common::pb::SetRoute *setroute);
    void DeleteUserSetCache(const std::string &user_id);

    bool UserNameValid(const std::string &user_name);
    bool UserIdValid(const std::string &user_id);
    bool BlobIdValid(const std::string &blob_id);
    bool EmailValid(const std::string &email);
    bool ParamLengthValid(const std::string &str, int limit);

    std::unordered_map<std::string, std::shared_ptr<brpc::Channel>>
            em_channel_map_;
    boost::shared_mutex channel_mutex_;

    std::unordered_map<std::string, std::shared_ptr<common::pb::SetRoute>>
            user_map_;
    boost::shared_mutex user_mutex_;
};

}  // namespace access
}  // namespace cyprestore

#endif  // CYPRESTORE_ACCESS_RESOURCE_MANAGER_H
