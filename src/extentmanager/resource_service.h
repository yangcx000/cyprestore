/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_RESOURCE_APIS_HANDLER_H_
#define CYPRESTORE_EXTENTMANAGER_RESOURCE_APIS_HANDLER_H_

#include "extentmanager/pb/resource.pb.h"

namespace cyprestore {
namespace extentmanager {

class ResourceServiceImpl : public pb::ResourceService {
public:
    ResourceServiceImpl() = default;
    virtual ~ResourceServiceImpl() = default;

    // User
    virtual void CreateUser(
            google::protobuf::RpcController *cntl_base,
            const pb::CreateUserRequest *request,
            pb::CreateUserResponse *response, google::protobuf::Closure *done);

    virtual void DeleteUser(
            google::protobuf::RpcController *cntl_base,
            const pb::DeleteUserRequest *request,
            pb::DeleteUserResponse *response, google::protobuf::Closure *done);

    virtual void UpdateUser(
            google::protobuf::RpcController *cntl_base,
            const pb::UpdateUserRequest *request,
            pb::UpdateUserResponse *response, google::protobuf::Closure *done);

    virtual void QueryUser(
            google::protobuf::RpcController *cntl_base,
            const pb::QueryUserRequest *request,
            pb::QueryUserResponse *response, google::protobuf::Closure *done);

    virtual void ListUsers(
            google::protobuf::RpcController *cntl_base,
            const pb::ListUsersRequest *request,
            pb::ListUsersResponse *response, google::protobuf::Closure *done);

    // Blob
    virtual void CreateBlob(
            google::protobuf::RpcController *cntl_base,
            const pb::CreateBlobRequest *request,
            pb::CreateBlobResponse *response, google::protobuf::Closure *done);

    virtual void DeleteBlob(
            google::protobuf::RpcController *cntl_base,
            const pb::DeleteBlobRequest *request,
            pb::DeleteBlobResponse *response, google::protobuf::Closure *done);

    virtual void RenameBlob(
            google::protobuf::RpcController *cntl_base,
            const pb::RenameBlobRequest *request,
            pb::RenameBlobResponse *response, google::protobuf::Closure *done);

    virtual void QueryBlob(
            google::protobuf::RpcController *cntl_base,
            const pb::QueryBlobRequest *request,
            pb::QueryBlobResponse *response, google::protobuf::Closure *done);

    virtual void ListBlobs(
            google::protobuf::RpcController *cntl_base,
            const pb::ListBlobsRequest *request,
            pb::ListBlobsResponse *response, google::protobuf::Closure *done);

    virtual void ResizeBlob(
            google::protobuf::RpcController *cntl_base,
            const pb::ResizeBlobRequest *request,
            pb::ResizeBlobResponse *response, google::protobuf::Closure *done);
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_RESOURCE_APIS_HANDLER_H_
