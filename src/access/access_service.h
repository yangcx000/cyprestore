/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_ACCESS_ACCESS_SERVICE_H
#define CYPRESTORE_ACCESS_ACCESS_SERVICE_H

#include <list>

#include "pb/access.pb.h"

namespace cyprestore {
namespace access {

class AccessServiceImpl : public pb::AccessService {
public:
    AccessServiceImpl() = default;
    virtual ~AccessServiceImpl() = default;

    virtual void CreateUser(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::CreateUserRequest *request,
            ::cyprestore::access::pb::CreateUserResponse *response,
            ::google::protobuf::Closure *done);
    virtual void DeleteUser(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::DeleteUserRequest *request,
            ::cyprestore::access::pb::DeleteUserResponse *response,
            ::google::protobuf::Closure *done);
    virtual void UpdateUser(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::UpdateUserRequest *request,
            ::cyprestore::access::pb::UpdateUserResponse *response,
            ::google::protobuf::Closure *done);
    virtual void QueryUser(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::QueryUserRequest *request,
            ::cyprestore::access::pb::QueryUserResponse *response,
            ::google::protobuf::Closure *done);
    virtual void ListUsers(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::ListUsersRequest *request,
            ::cyprestore::access::pb::ListUsersResponse *response,
            ::google::protobuf::Closure *done);
    virtual void CreateBlob(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::CreateBlobRequest *request,
            ::cyprestore::access::pb::CreateBlobResponse *response,
            ::google::protobuf::Closure *done);
    virtual void DeleteBlob(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::DeleteBlobRequest *request,
            ::cyprestore::access::pb::DeleteBlobResponse *response,
            ::google::protobuf::Closure *done);
    virtual void RenameBlob(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::RenameBlobRequest *request,
            ::cyprestore::access::pb::RenameBlobResponse *response,
            ::google::protobuf::Closure *done);
    virtual void ResizeBlob(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::ResizeBlobRequest *request,
            ::cyprestore::access::pb::ResizeBlobResponse *response,
            ::google::protobuf::Closure *done);
    virtual void QueryBlob(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::QueryBlobRequest *request,
            ::cyprestore::access::pb::QueryBlobResponse *response,
            ::google::protobuf::Closure *done);
    virtual void ListBlobs(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::ListBlobsRequest *request,
            ::cyprestore::access::pb::ListBlobsResponse *response,
            ::google::protobuf::Closure *done);
    virtual void QuerySet(
            ::google::protobuf::RpcController *controller,
            const ::cyprestore::access::pb::QuerySetRequest *request,
            ::cyprestore::access::pb::QuerySetResponse *response,
            ::google::protobuf::Closure *done);
};

}  // namespace access
}  // namespace cyprestore

#endif  // CYPRESTORE_ACCESS_ACCESS_SERVICE_H
