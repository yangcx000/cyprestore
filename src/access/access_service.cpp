/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#include "access_service.h"

#include <brpc/server.h>
#include <butil/logging.h>

#include <list>

#include "access_manager.h"
#include "common/error_code.h"
#include "common/pb/types.pb.h"
#include "pb/access.pb.h"
#include "resource_manager.h"

namespace cyprestore {
namespace access {

void AccessServiceImpl::CreateUser(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::CreateUserRequest *request,
        ::cyprestore::access::pb::CreateUserResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    std::string user_id;
    auto status = Access::GlobalInstance().get_resource_mgr()->CreateUser(
            request->user_name(), request->email(), request->comments(),
            request->set_id(), request->pool_id(), request->pool_type(),
            &user_id);
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "CreateUser failed: " << status.message();
        return;
    }
    response->set_user_id(user_id);
    LOG(INFO) << "CreateUser succeed";
}

void AccessServiceImpl::DeleteUser(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::DeleteUserRequest *request,
        ::cyprestore::access::pb::DeleteUserResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    auto status = Access::GlobalInstance().get_resource_mgr()->DeleteUser(
            request->user_id());
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "DeleteUser failed: " << status.message();
        return;
    }
    LOG(INFO) << "DeleteUser succeed";
}

void AccessServiceImpl::UpdateUser(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::UpdateUserRequest *request,
        ::cyprestore::access::pb::UpdateUserResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    auto status = Access::GlobalInstance().get_resource_mgr()->UpdateUser(
            request->user_id(), request->new_name(), request->new_email(),
            request->new_comments());
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "Update user failed: " << status.message();
        return;
    }
    LOG(INFO) << "Update user succeed";
}

void AccessServiceImpl::QueryUser(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::QueryUserRequest *request,
        ::cyprestore::access::pb::QueryUserResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    common::pb::User userinfo;
    auto status = Access::GlobalInstance().get_resource_mgr()->QueryUser(
            request->user_id(), &userinfo);
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "QueryUser failed: " << status.message();
        return;
    }
    response->mutable_userinfo()->CopyFrom(userinfo);
    LOG(INFO) << "QueryUser succeed";
}

void AccessServiceImpl::ListUsers(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::ListUsersRequest *request,
        ::cyprestore::access::pb::ListUsersResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    std::vector<common::pb::User> users;
    auto status = Access::GlobalInstance().get_resource_mgr()->ListUsers(
            request->set_id(), &users);
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "ListUsers failed: " << status.message();
        return;
    }
    for (auto &user : users) {
        *response->add_users() = user;
    }

    LOG(INFO) << "ListUsers succeed";
}

void AccessServiceImpl::CreateBlob(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::CreateBlobRequest *request,
        ::cyprestore::access::pb::CreateBlobResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    std::string blob_id;
    auto status = Access::GlobalInstance().get_resource_mgr()->CreateBlob(
            request->user_id(), request->blob_name(), request->blob_size(),
            request->blob_type(), request->instance_id(), request->blob_desc(),
            request->pool_id(), &blob_id);
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "CreateBlob failed: " << status.message();
        return;
    }
    response->set_blob_id(blob_id);
    LOG(INFO) << "CreateBlob succeed";
}

void AccessServiceImpl::DeleteBlob(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::DeleteBlobRequest *request,
        ::cyprestore::access::pb::DeleteBlobResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    auto status = Access::GlobalInstance().get_resource_mgr()->DeleteBlob(
            request->user_id(), request->blob_id());
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "DeleteBlob failed: " << status.message();
        return;
    }
    LOG(INFO) << "DeleteBlob succeed";
}

void AccessServiceImpl::RenameBlob(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::RenameBlobRequest *request,
        ::cyprestore::access::pb::RenameBlobResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    auto status = Access::GlobalInstance().get_resource_mgr()->RenameBlob(
            request->user_id(), request->blob_id(), request->new_name());
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "RenamBlob failed: " << status.message();
        return;
    }
    LOG(INFO) << "RenameBlob succeed";
}

void AccessServiceImpl::ResizeBlob(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::ResizeBlobRequest *request,
        ::cyprestore::access::pb::ResizeBlobResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    auto status = Access::GlobalInstance().get_resource_mgr()->ResizeBlob(
            request->user_id(), request->blob_id(), request->new_size());
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "ResizeBlob failed: " << status.message();
        return;
    }
    LOG(INFO) << "ResizeBlob succeed";
}

void AccessServiceImpl::QueryBlob(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::QueryBlobRequest *request,
        ::cyprestore::access::pb::QueryBlobResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    common::pb::Blob blob;
    auto status = Access::GlobalInstance().get_resource_mgr()->QueryBlob(
            request->user_id(), request->blob_id(), &blob);
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "QueryBlob failed: " << status.message();
        return;
    }
    response->mutable_blob_info()->CopyFrom(blob);
    LOG(INFO) << "QueryBlob succeed";
}

void AccessServiceImpl::ListBlobs(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::ListBlobsRequest *request,
        ::cyprestore::access::pb::ListBlobsResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    std::vector<common::pb::Blob> blobs;
    auto status = Access::GlobalInstance().get_resource_mgr()->ListBlobs(
            request->user_id(), &blobs);
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "ListBlobs failed: " << status.message();
    }
    for (auto &blob : blobs) {
        *response->add_blobs() = blob;
    }
    LOG(INFO) << "ListBlob succeed";
}

void AccessServiceImpl::QuerySet(
        ::google::protobuf::RpcController *controller,
        const ::cyprestore::access::pb::QuerySetRequest *request,
        ::cyprestore::access::pb::QuerySetResponse *response,
        ::google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    common::pb::SetRoute setroute;
    auto status = Access::GlobalInstance().get_set_mgr()->QuerySet(
            request->user_id(), &setroute);
    *response->mutable_status() = status;
    if (status.code() != common::CYPRE_OK) {
        LOG(ERROR) << "QuerySet failed: " << status.message();
        return;
    }

    response->mutable_setroute()->CopyFrom(setroute);
    LOG(INFO) << "QuerySet succeed";
}

}  // namespace access
}  // namespace cyprestore
