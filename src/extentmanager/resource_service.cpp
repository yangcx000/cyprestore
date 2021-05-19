/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "resource_service.h"

#include <brpc/server.h>
#include <butil/logging.h>

#include "common/error_code.h"
#include "common/pb/types.pb.h"
#include "extent_manager.h"
#include "utils/pb_transfer.h"

namespace cyprestore {
namespace extentmanager {

void ResourceServiceImpl::CreateUser(
        google::protobuf::RpcController *cntl_base,
        const pb::CreateUserRequest *request, pb::CreateUserResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->user_name().empty() || request->email().empty()) {
        LOG(ERROR) << "Create user failed, username or email empty"
                   << ", username:" << request->user_name()
                   << ", email:" << request->email();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("username or email empty");
        return;
    }

    // 2. create user
    int set_id = ExtentManager::GlobalInstance().get_set_id();
    std::string user_id;
    auto status = ExtentManager::GlobalInstance().get_user_mgr()->create_user(
            request->user_name(), request->email(), request->comments(), set_id,
            &user_id);
    if (!status.ok()) {
        LOG(ERROR) << "Create user failed, username: " << request->user_name();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    response->set_user_id(user_id);

    LOG(INFO) << "Create user succeed, username: " << request->user_name()
              << ", userid: " << user_id;
}

void ResourceServiceImpl::DeleteUser(
        google::protobuf::RpcController *cntl_base,
        const pb::DeleteUserRequest *request, pb::DeleteUserResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->user_id().empty()) {
        LOG(ERROR) << "Delete user failed, userid empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("userid empty");
        return;
    }

    // 2. delete user
    auto status =
            ExtentManager::GlobalInstance().get_user_mgr()->soft_delete_user(
                    request->user_id());
    if (!status.ok()) {
        LOG(ERROR) << "Delete user failed, user_id:" << request->user_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Delete user succeed, user_id:" << request->user_id();
}

void ResourceServiceImpl::UpdateUser(
        google::protobuf::RpcController *cntl_base,
        const pb::UpdateUserRequest *request, pb::UpdateUserResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->user_id().empty()
        || (request->new_name().empty() && request->new_email().empty()
            && request->new_comments().empty())) {
        LOG(ERROR) << "Update user failed, userid or new information empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message(
                "userid or new information empty");
        return;
    }

    // 2. update user
    auto status = ExtentManager::GlobalInstance().get_user_mgr()->update_user(
            request->user_id(), request->new_name(), request->new_email(),
            request->new_comments());
    if (!status.ok()) {
        LOG(ERROR) << "Update user failed, user_id:" << request->user_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Update user: " << request->user_id() << " succeed";
}

void ResourceServiceImpl::QueryUser(
        google::protobuf::RpcController *cntl_base,
        const pb::QueryUserRequest *request, pb::QueryUserResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->user_id().empty()) {
        LOG(ERROR) << "Query user failed, userid empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("userid empty");
        return;
    }

    // 2. query user
    common::pb::User user;
    auto status = ExtentManager::GlobalInstance().get_user_mgr()->query_user(
            request->user_id(), &user);
    if (!status.ok()) {
        LOG(ERROR) << "Query user failed, user_id:" << request->user_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    *response->mutable_user() = user;
    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Query user " << request->user_id() << " succeed";
}

void ResourceServiceImpl::ListUsers(
        google::protobuf::RpcController *cntl_base,
        const pb::ListUsersRequest *request, pb::ListUsersResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    // 1. list users
    std::vector<common::pb::User> users;
    auto status =
            ExtentManager::GlobalInstance().get_user_mgr()->list_users(&users);
    for (uint32_t i = 0; i < users.size(); i++) {
        *response->add_users() = users[i];
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "List users succeed";
}

// Blob
void ResourceServiceImpl::CreateBlob(
        google::protobuf::RpcController *cntl_base,
        const pb::CreateBlobRequest *request, pb::CreateBlobResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->user_id().empty() || request->blob_name().empty()
        || request->instance_id().empty()
        || !blobsize_valid(request->blob_size())
        || !utils::blobtype_valid(request->blob_type())) {
        LOG(ERROR) << "Create blob failed, argument invalid"
                   << ", user_id:" << request->user_id()
                   << ", blob_name:" << request->blob_name()
                   << ", instance_id:" << request->instance_id()
                   << ", blob_size:" << request->blob_size()
                   << ", blob_type:" << static_cast<int>(request->blob_type());
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("argument invalid");
        return;
    }

    // 2. check user
    auto exist = ExtentManager::GlobalInstance().get_user_mgr()->user_exist(
            request->user_id());
    if (!exist) {
        LOG(ERROR) << "Create blob failed, can't find user, user_id:"
                   << request->user_id()
                   << ", blob_name:" << request->blob_name();
        response->mutable_status()->set_code(common::CYPRE_EM_USER_NOT_FOUND);
        response->mutable_status()->set_message("User not exist");
        return;
    }

    // 3. check pool
    BlobType blob_type = utils::FromPbBlobType(request->blob_type());
    std::string pool_id = request->pool_id();
    if (pool_id.empty()) {
        pool_id = ExtentManager::GlobalInstance().get_pool_mgr()->allocate_pool(
                blob_type);
    }

    if (pool_id.empty()) {
        LOG(ERROR) << "Create blob failed, can't find valid pool"
                   << ", blob_name:" << request->blob_name();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("can't find valid pool");
        return;
    }

    // 4. create blob
    std::string blob_id;
    std::vector<std::string> rg_ids;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->create_blob(
            pool_id, request->blob_name(), request->blob_size(), blob_type,
            request->blob_desc(), request->user_id(), request->instance_id(),
            &blob_id, &rg_ids);
    if (!status.ok()) {
        LOG(ERROR) << "Create blob failed.";
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    // 5. update user info
    ExtentManager::GlobalInstance().get_user_mgr()->add_pool(
            request->user_id(), pool_id);

    // 6. create router
    auto extent_size =
            ExtentManager::GlobalInstance().get_pool_mgr()->get_extent_size();
    int extent_num = int(request->blob_size() / extent_size);
    status = ExtentManager::GlobalInstance().get_router_mgr()->create_router(
            pool_id, blob_id, 1, extent_num, rg_ids);
    if (!status.ok()) {
        LOG(ERROR) << "Create blob failed.";
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    response->set_blob_id(blob_id);

    LOG(INFO) << "Create blob succeed, user_id:" << request->user_id()
              << ", blob_name:" << request->blob_name()
              << ", blob_id:" << blob_id << ", pool_id:" << pool_id;
}

void ResourceServiceImpl::ResizeBlob(
        google::protobuf::RpcController *cntl_base,
        const pb::ResizeBlobRequest *request, pb::ResizeBlobResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->user_id().empty() || request->blob_id().empty()
        || !blobsize_valid(request->new_size())) {
        LOG(ERROR) << "Resize blob failed, userid or blobid empty or blobsize "
                      "invalid"
                   << ", user_id:" << request->user_id()
                   << ", blob_id:" << request->blob_id()
                   << ", new_size:" << request->new_size();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message(
                "usreid or or blobid or blobsize invalid");
        return;
    }

    // 2. check user
    auto exist = ExtentManager::GlobalInstance().get_user_mgr()->user_exist(
            request->user_id());
    if (!exist) {
        LOG(ERROR) << "Resize blob failed, can't find user, user_id:"
                   << request->user_id();
        response->mutable_status()->set_code(common::CYPRE_EM_USER_NOT_FOUND);
        response->mutable_status()->set_message("not found user");
        return;
    }

    // 3. resize blob
    uint64_t old_size;
    std::string pool_id;
    std::vector<std::string> rg_ids;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->resize_blob(
            request->user_id(), request->blob_id(), request->new_size(),
            &pool_id, &old_size, &rg_ids);
    if (!status.ok()) {
        LOG(ERROR) << "Resize blob failed, blob_id:" << request->blob_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    // 4. create router
    auto extent_size =
            ExtentManager::GlobalInstance().get_pool_mgr()->get_extent_size();
    int new_extent_num = int(request->new_size() / extent_size);
    int old_extent_num = int(old_size / extent_size);
    status = ExtentManager::GlobalInstance().get_router_mgr()->create_router(
            pool_id, request->blob_id(), old_extent_num + 1, new_extent_num,
            rg_ids);
    if (!status.ok()) {
        LOG(ERROR) << "Create blob failed.";
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Resize blob succeed, blob_id:" << request->blob_id()
              << ", new_size:" << request->new_size();
}

void ResourceServiceImpl::DeleteBlob(
        google::protobuf::RpcController *cntl_base,
        const pb::DeleteBlobRequest *request, pb::DeleteBlobResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->user_id().empty() || request->blob_id().empty()) {
        LOG(ERROR) << "Delete blob failed, userid or blobid empty"
                   << ", user_id: " << request->user_id()
                   << ", blob_id: " << request->blob_id();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("userid or blobid empty");
        return;
    }

    // 2. check user
    auto exist = ExtentManager::GlobalInstance().get_user_mgr()->user_exist(
            request->user_id());
    if (!exist) {
        LOG(ERROR) << "Delete blob failed, can't find user, user_id:"
                   << request->user_id();
        response->mutable_status()->set_code(common::CYPRE_EM_USER_NOT_FOUND);
        response->mutable_status()->set_message("user not found");
        return;
    }

    // 3. delete blob
    auto status =
            ExtentManager::GlobalInstance().get_pool_mgr()->soft_delete_blob(
                    request->user_id(), request->blob_id());
    if (!status.ok()) {
        LOG(ERROR) << "Delete blob failed, blob_id:" << request->blob_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Delete blob succeed, blob_id:" << request->blob_id();
}

void ResourceServiceImpl::RenameBlob(
        google::protobuf::RpcController *cntl_base,
        const pb::RenameBlobRequest *request, pb::RenameBlobResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->user_id().empty() || request->blob_id().empty()
        || request->new_name().empty()) {
        LOG(ERROR) << "Rename blob failed, userid or blobid or newname empty"
                   << ", user_id:" << request->user_id()
                   << ", blob_id:" << request->blob_id()
                   << ", new_name:" << request->new_name();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message(
                "userid or blobid or newname empty");
        return;
    }

    // 2. check user
    auto exist = ExtentManager::GlobalInstance().get_user_mgr()->user_exist(
            request->user_id());
    if (!exist) {
        LOG(ERROR) << "Rename blob failed, can't find user, user_id:"
                   << request->user_id();
        response->mutable_status()->set_code(common::CYPRE_EM_USER_NOT_FOUND);
        response->mutable_status()->set_message("user not found");
        return;
    }

    // 3. rename blob
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->rename_blob(
            request->user_id(), request->blob_id(), request->new_name());
    if (!status.ok()) {
        LOG(ERROR) << "Rename blob failed, blob_id:" << request->blob_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());

    LOG(INFO) << "Rename blob succeed"
              << ", blob_id:" << request->blob_id()
              << ", newname:" << request->new_name();
}

void ResourceServiceImpl::QueryBlob(
        google::protobuf::RpcController *cntl_base,
        const pb::QueryBlobRequest *request, pb::QueryBlobResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->blob_id().empty()) {
        LOG(ERROR) << "Query blob failed, blobid empty, blob_id:"
                   << request->blob_id();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("blob_id empty");
        return;
    }

    // 2. query blob
    common::pb::Blob blob;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->query_blob(
            request->blob_id(), false, &blob);
    if (!status.ok()) {
        LOG(ERROR) << "Query blob failed, blob id: " << request->blob_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    *(response->mutable_blob()) = blob;
    response->set_extent_size(GlobalConfig().extentmanager().extent_size);
    response->set_max_block_size(GlobalConfig().extentmanager().max_block_size);
    response->mutable_status()->set_code(status.code());
}

void ResourceServiceImpl::ListBlobs(
        google::protobuf::RpcController *cntl_base,
        const pb::ListBlobsRequest *request, pb::ListBlobsResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->user_id().empty()) {
        LOG(ERROR) << "List blob failed, userid empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("userid empty");
        return;
    }

    // 2. check user
    common::pb::User user;
    auto exist = ExtentManager::GlobalInstance().get_user_mgr()->user_exist(
            request->user_id());
    if (!exist) {
        LOG(ERROR) << "List blob failed, can't find user, user_id:"
                   << request->user_id();
        response->mutable_status()->set_code(common::CYPRE_EM_USER_NOT_FOUND);
        response->mutable_status()->set_message("user not found");
        return;
    }

    // 3. list blobs
    std::vector<common::pb::Blob> blobs;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->list_blobs(
            request->user_id(), &blobs);
    for (uint32_t i = 0; i < blobs.size(); i++) {
        *response->add_blobs() = blobs[i];
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "List blobs succeed, user_id:" << request->user_id();
}

}  // namespace extentmanager
}  // namespace cyprestore
