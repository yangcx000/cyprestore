/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "resource_manager.h"

#include "access_manager.h"
#include "common/error_code.h"
#include "extentmanager/pb/resource.pb.h"
#include "utils/regex_util.h"

namespace cyprestore {
namespace access {

const int USERNAME_LIMIT = 64;
const int USERID_LIMIT = 64;
const int EMAIL_LIMIT = 64;
const int BLOBID_LIMIT = 128;

bool ResourceMgr::UserNameValid(const std::string &user_name) {
    return utils::RegexUtil::UserNameValid(user_name)
           && user_name.length() <= USERNAME_LIMIT;
}

bool ResourceMgr::UserIdValid(const std::string &user_id) {
    return utils::RegexUtil::UserIdValid(user_id)
           && user_id.length() <= USERID_LIMIT;
}

bool ResourceMgr::BlobIdValid(const std::string &blob_id) {
    return utils::RegexUtil::BlobIdValid(blob_id)
           && blob_id.length() <= BLOBID_LIMIT;
}

bool ResourceMgr::EmailValid(const std::string &email) {
    return utils::RegexUtil::EmailValid(email) && email.length() <= EMAIL_LIMIT;
}

bool ResourceMgr::ParamLengthValid(const std::string &str, int limit) {
    return str.length() <= limit;
}

std::shared_ptr<brpc::Channel>
ResourceMgr::GetChannel(const std::string &em_ip, int em_port) {
    {
        boost::shared_lock<boost::shared_mutex> lock(channel_mutex_);
        auto iter = em_channel_map_.find(em_ip);
        if (iter != em_channel_map_.end()) {
            return iter->second;
        }
    }
    return InitChannel(em_ip, em_port);
}

std::shared_ptr<brpc::Channel>
ResourceMgr::InitChannel(const std::string &em_ip, int em_port) {
    boost::unique_lock<boost::shared_mutex> lock(channel_mutex_);

    // double check
    auto iter = em_channel_map_.find(em_ip);
    if (iter != em_channel_map_.end()) {
        return iter->second;
    }

    std::shared_ptr<brpc::Channel> em_channel;
    em_channel.reset(new brpc::Channel());
    std::stringstream ss(std::ios_base::app | std::ios_base::out);
    ss << em_ip << ":" << em_port;
    LOG(INFO) << "extent manager addr: " << ss.str();
    if (em_channel->Init(ss.str().c_str(), NULL) != 0) {
        LOG(ERROR) << "Couldn't connect extentmanager: " << ss.str();
        return nullptr;
    }
    em_channel_map_[em_ip] = em_channel;
    return em_channel;
}

Status ResourceMgr::QueryUserSet(
        const std::string &user_id, common::pb::SetRoute *setroute) {
    // 1. look in cache
    Status status;
    {
        boost::shared_lock<boost::shared_mutex> lock(user_mutex_);
        auto iter = user_map_.find(user_id);
        if (iter != user_map_.end()) {
            LOG(INFO) << "hit in cache, user id: " << user_id;
            setroute->CopyFrom(*(iter->second.get()));
            status.set_code(common::CYPRE_OK);
            return status;
        }
    }
    // 2. look in remote
    status =
            Access::GlobalInstance().get_set_mgr()->QuerySet(user_id, setroute);
    if (status.code() == common::CYPRE_OK) {
        boost::unique_lock<boost::shared_mutex> lock(user_mutex_);
        std::unique_ptr<common::pb::SetRoute> routeptr(
                new common::pb::SetRoute(*setroute));
        user_map_[user_id] = std::move(routeptr);
        LOG(INFO) << "hit in remote, user id: " << user_id;
    }
    return status;
}

void ResourceMgr::DeleteUserSetCache(const std::string &user_id) {
    boost::unique_lock<boost::shared_mutex> lock(user_mutex_);
    LOG(INFO) << "delete cache, user id: " << user_id;
    user_map_.erase(user_id);
}

Status ResourceMgr::CreateUser(
        const std::string &user_name, const std::string &email,
        const std::string &comments, int set_id, const std::string &pool_id,
        common::pb::PoolType type, std::string *user_id) {
    // 1. check param
    Status status;
    if (!UserNameValid(user_name) || !EmailValid(email)
        || !ParamLengthValid(comments, 128)) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user name or email or comments invalid");
        LOG(ERROR) << "Create user failed, argument invalid"
                   << ", user_name: " << user_name << ", email: " << email
                   << ", comments: " << comments;
        return status;
    }
    // 2. allocate set
    common::pb::SetRoute setroute;
    if (set_id > 0) {
        auto ret = Access::GlobalInstance().get_set_mgr()->QuerySetById(
                set_id, &setroute);
        if (ret.code() != common::CYPRE_OK) {
            return ret;
        }
    } else {
        auto ret = Access::GlobalInstance().get_set_mgr()->AllocateSet(
                user_name, type, &setroute);
        if (ret.code() != common::CYPRE_OK) {
            return ret;
        }
    }

    brpc::Controller cntl;
    extentmanager::pb::CreateUserRequest request;
    extentmanager::pb::CreateUserResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    request.set_user_name(user_name);
    request.set_email(email);
    request.set_comments(comments);
    stub.CreateUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send create user request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());

    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Create user failed: " << response.status().message();
        return response.status();
    }

    *user_id = response.user_id();
    status.set_code(common::CYPRE_OK);
    return status;
}

Status ResourceMgr::DeleteUser(const std::string &user_id) {
    // 1. check param
    Status status;
    if (!UserIdValid(user_id)) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user id invalid");
        LOG(ERROR) << "Delete user failed, argument invalid"
                   << ", user_id: " << user_id;
        return status;
    }

    // 2. query set
    common::pb::SetRoute setroute;
    auto ret = QueryUserSet(user_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    brpc::Controller cntl;
    extentmanager::pb::DeleteUserRequest request;
    extentmanager::pb::DeleteUserResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    request.set_user_id(user_id);
    stub.DeleteUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send delete user request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Delete user failed: " << response.status().message();
        return response.status();
    }

    DeleteUserSetCache(user_id);
    status.set_code(common::CYPRE_OK);
    return status;
}

Status ResourceMgr::UpdateUser(
        const std::string &user_id, const std::string &new_name,
        const std::string &new_email, const std::string &new_comments) {
    // 1. check param
    Status status;
    if (!UserIdValid(user_id)
        || (!UserNameValid(new_name) && !EmailValid(new_email)
            && !ParamLengthValid(new_comments, 128))) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user id or update info invalid");
        LOG(ERROR) << "Update user failed, argument invalid"
                   << ", user_id: " << user_id << ", user_name: " << new_name
                   << ", user_email: " << new_email
                   << ", comments: " << new_comments;
        return status;
    }

    // 2. query set
    common::pb::SetRoute setroute;
    auto ret = QueryUserSet(user_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    brpc::Controller cntl;
    extentmanager::pb::UpdateUserRequest request;
    extentmanager::pb::UpdateUserResponse response;
    request.set_user_id(user_id);
    request.set_new_name(new_name);
    request.set_new_email(new_email);
    request.set_new_comments(new_comments);
    stub.UpdateUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send rename user request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Update user failed: " << response.status().message();
        return response.status();
    }

    status.set_code(common::CYPRE_OK);
    return status;
}

Status
ResourceMgr::QueryUser(const std::string &user_id, common::pb::User *userinfo) {
    // 1. check param
    Status status;
    if (!UserIdValid(user_id)) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user id invalid");
        LOG(ERROR) << "Query user failed, argument invalid"
                   << ", user_id: " << user_id;
        return status;
    }

    // 2. query set
    common::pb::SetRoute setroute;
    auto ret = QueryUserSet(user_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    brpc::Controller cntl;
    extentmanager::pb::QueryUserRequest request;
    extentmanager::pb::QueryUserResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    request.set_user_id(user_id);
    stub.QueryUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send query user request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Query user failed: " << response.status().message();
        return response.status();
    }

    *userinfo = response.user();
    status.set_code(common::CYPRE_OK);
    return status;
}

Status
ResourceMgr::ListUsers(int set_id, std::vector<common::pb::User> *users) {
    common::pb::SetRoute setroute;
    auto ret = Access::GlobalInstance().get_set_mgr()->QuerySetById(
            set_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    Status status;
    brpc::Controller cntl;
    extentmanager::pb::ListUsersRequest request;
    extentmanager::pb::ListUsersResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }

    extentmanager::pb::ResourceService_Stub stub(channel.get());
    stub.ListUsers(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send list users request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "List users failed: " << response.status().message();
        return response.status();
    }
    for (int i = 0; i < response.users_size(); i++) {
        users->push_back(response.users(i));
    }
    status.set_code(common::CYPRE_OK);
    return status;
}

Status ResourceMgr::CreateBlob(
        const std::string &user_id, const std::string &blob_name,
        uint64_t blob_size, common::pb::BlobType type,
        const std::string &instance_id, const std::string &blob_desc,
        const std::string &pool_id, std::string *blob_id) {
    // 1. check param
    Status status;
    if (!UserIdValid(user_id) || !ParamLengthValid(blob_name, 128)
        || !ParamLengthValid(blob_desc, 256)) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user id or blob name or blob desc invalid");
        LOG(ERROR) << "Create blob failed, argument invalid"
                   << ", user_id: " << user_id << ", blob_name: " << blob_name
                   << ", blob_desc: " << blob_desc;
        return status;
    }

    // 2. query set
    common::pb::SetRoute setroute;
    auto ret = QueryUserSet(user_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    brpc::Controller cntl;
    extentmanager::pb::CreateBlobRequest request;
    extentmanager::pb::CreateBlobResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    request.set_user_id(user_id);
    request.set_blob_name(blob_name);
    request.set_blob_size(blob_size);
    request.set_blob_type(type);
    request.set_blob_desc(blob_desc);
    request.set_pool_id(pool_id);
    request.set_instance_id(instance_id);

    stub.CreateBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send create blob request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Create blob failed: " << response.status().message();
        return response.status();
    }

    *blob_id = response.blob_id();
    status.set_code(common::CYPRE_OK);
    return status;
}

Status ResourceMgr::DeleteBlob(
        const std::string &user_id, const std::string &blob_id) {
    // 1. check param
    Status status;
    if (!UserIdValid(user_id) || !BlobIdValid(blob_id)) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user id or blob id invalid");
        LOG(ERROR) << "Delete blob failed, argument invalid"
                   << ", user_id: " << user_id << ", blob_id: " << blob_id;
        return status;
    }

    // 2. query set
    common::pb::SetRoute setroute;
    auto ret = QueryUserSet(user_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    brpc::Controller cntl;
    extentmanager::pb::DeleteBlobRequest request;
    extentmanager::pb::DeleteBlobResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    request.set_user_id(user_id);
    request.set_blob_id(blob_id);
    stub.DeleteBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send delete blob request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Delete blob failed: " << response.status().message();
        return response.status();
    }

    status.set_code(common::CYPRE_OK);
    return status;
}

Status ResourceMgr::RenameBlob(
        const std::string &user_id, const std::string &blob_id,
        const std::string &new_name) {
    // 1. check param
    Status status;
    if (!UserIdValid(user_id) || !BlobIdValid(blob_id)
        || !ParamLengthValid(new_name, 128)) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user id or blob id or new name invalid");
        LOG(ERROR) << "Rename blob failed, argument invalid"
                   << ", user_id: " << user_id << ", blob_id: " << blob_id
                   << ", blob_name: " << new_name;
        return status;
    }

    // 2. query set
    common::pb::SetRoute setroute;
    auto ret = QueryUserSet(user_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    brpc::Controller cntl;
    extentmanager::pb::RenameBlobRequest request;
    extentmanager::pb::RenameBlobResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    request.set_user_id(user_id);
    request.set_blob_id(blob_id);
    request.set_new_name(new_name);
    stub.RenameBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send rename blob request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Rename blob failed: " << response.status().message();
        return response.status();
    }

    status.set_code(common::CYPRE_OK);
    return status;
}

Status ResourceMgr::ResizeBlob(
        const std::string &user_id, const std::string &blob_id,
        uint64_t blob_size) {
    // 1. check param
    Status status;
    if (!UserIdValid(user_id) || !BlobIdValid(blob_id)) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user id or blob id size invalid");
        LOG(ERROR) << "Resize blob failed, argument invalid"
                   << ", user_id: " << user_id << ", blob_id: " << blob_id;
        return status;
    }

    // 2. query set
    common::pb::SetRoute setroute;
    auto ret = QueryUserSet(user_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    brpc::Controller cntl;
    extentmanager::pb::ResizeBlobRequest request;
    extentmanager::pb::ResizeBlobResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    request.set_user_id(user_id);
    request.set_blob_id(blob_id);
    request.set_new_size(blob_size);
    stub.ResizeBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send query set request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Query set failed: " << response.status().message();
        return response.status();
    }

    status.set_code(common::CYPRE_OK);
    return status;
}

Status ResourceMgr::QueryBlob(
        const std::string &user_id, const std::string &blob_id,
        common::pb::Blob *blob) {
    // 1. check param
    Status status;
    if (!UserIdValid(user_id) || !BlobIdValid(blob_id)) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user id or blob id invalid");
        LOG(ERROR) << "Query blob failed, argument invalid"
                   << ", user_id: " << user_id << ", blob_id: " << blob_id;
        return status;
    }

    // 2. query set
    common::pb::SetRoute setroute;
    auto ret = QueryUserSet(user_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    brpc::Controller cntl;
    extentmanager::pb::QueryBlobRequest request;
    extentmanager::pb::QueryBlobResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    request.set_blob_id(blob_id);
    stub.QueryBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send query set request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Query set failed: " << response.status().message();
        return response.status();
    }

    *blob = response.blob();
    status.set_code(common::CYPRE_OK);
    return status;
}

Status ResourceMgr::ListBlobs(
        const std::string &user_id, std::vector<common::pb::Blob> *blobs) {
    // 1. check param
    Status status;
    if (!UserIdValid(user_id)) {
        status.set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        status.set_message("user id invalid");
        LOG(ERROR) << "List blobs failed, argument invalid"
                   << ", user_id: " << user_id;
        return status;
    }

    // 2. query set
    common::pb::SetRoute setroute;
    auto ret = QueryUserSet(user_id, &setroute);
    if (ret.code() != common::CYPRE_OK) {
        return ret;
    }

    brpc::Controller cntl;
    extentmanager::pb::ListBlobsRequest request;
    extentmanager::pb::ListBlobsResponse response;
    auto channel = GetChannel(setroute.ip(), setroute.port());
    if (channel == nullptr) {
        status.set_code(common::CYPRE_AC_CHANNEL_ERROR);
        status.set_message("channel not available");
        return status;
    }
    extentmanager::pb::ResourceService_Stub stub(channel.get());

    request.set_user_id(user_id);
    stub.ListBlobs(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send list blobs request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "List blobs failed: " << response.status().message();
        return response.status();
    }

    for (int i = 0; i < response.blobs_size(); i++) {
        blobs->push_back(response.blobs(i));
    }
    status.set_code(common::CYPRE_OK);
    return status;
}

}  // namespace access
}  // namespace cyprestore
