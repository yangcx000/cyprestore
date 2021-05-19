/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#include "rpcclient_access.h"

namespace cyprestore {

RpcClientAccess::RpcClientAccess() {}

RpcClientAccess::~RpcClientAccess() {}

StatusCode RpcClientAccess::CreateUser(
        const std::string &user_name, const std::string &email,
        const std::string &comments, int32_t set_id, const std::string &pool_id,
        cyprestore::common::pb::PoolType pool_type, std::string &user_id,
        std::string &errmsg) {
    cyprestore::access::pb::CreateUserRequest request;
    cyprestore::access::pb::CreateUserResponse response;
    request.set_user_name(user_name);
    request.set_email(email);
    request.set_comments(comments);
    request.set_set_id(set_id);
    request.set_pool_id(pool_id);
    request.set_pool_type(pool_type);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.CreateUser(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.CreateUser", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    user_id = response.user_id();
    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode
RpcClientAccess::DeleteUser(const std::string &user_id, std::string &errmsg) {

    cyprestore::access::pb::DeleteUserRequest request;
    cyprestore::access::pb::DeleteUserResponse response;
    request.set_user_id(user_id);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.DeleteUser(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.DeleteUser", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientAccess::RenameUser(
        const std::string &user_id, const std::string &user_name,
        std::string &errmsg) {
    cyprestore::access::pb::RenameUserRequest request;
    cyprestore::access::pb::RenameUserResponse response;
    request.set_user_id(user_id);
    request.set_new_name(user_name);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.RenameUser(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.RenameUser", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientAccess::QueryUser(
        const std::string &user_id, User &userinfo, std::string &errmsg) {

    cyprestore::access::pb::QueryUserRequest request;
    cyprestore::access::pb::QueryUserResponse response;
    request.set_user_id(user_id);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.QueryUser(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.QueryUser", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    userinfo = response.userinfo();
    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientAccess::ListUsers(
        int set_id, const int from, const int count, std::list<User> &Users,
        std::string &errmsg) {

    cyprestore::access::pb::ListUsersRequest request;
    cyprestore::access::pb::ListUsersResponse response;
    request.set_set_id(set_id);
    request.set_from(from);
    request.set_count(count);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.ListUsers(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.ListUsers", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    Users.clear();
    for (int i = 0; i < response.users_size(); i++)
        Users.push_back(response.users(i));
    return (::cyprestore::common::pb::STATUS_OK);
}

// blob
StatusCode RpcClientAccess::CreateBlob(
        const std::string &user_id, const std::string &blob_name,
        uint64_t blob_size, const BlobType blob_type,
        const std::string &instance_id, const std::string &blob_desc,
        const std::string &pool_id, std::string &blob_id, std::string &errmsg) {

    cyprestore::access::pb::CreateBlobRequest request;
    cyprestore::access::pb::CreateBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_name(blob_name);
    request.set_blob_size(blob_size);
    request.set_blob_type(blob_type);
    request.set_instance_id(instance_id);
    request.set_blob_desc(blob_desc);
    request.set_pool_id(pool_id);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.CreateBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.CreateBlob", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    blob_id = response.blob_id();
    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientAccess::DeleteBlob(
        const std::string &user_id, const std::string &blob_id,
        std::string &errmsg) {
    cyprestore::access::pb::DeleteBlobRequest request;
    cyprestore::access::pb::DeleteBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_id(blob_id);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.DeleteBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.DeleteBlob", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientAccess::RenameBlob(
        const std::string &user_id, const std::string &blob_id,
        const std::string &new_name, std::string &errmsg) {

    cyprestore::access::pb::RenameBlobRequest request;
    cyprestore::access::pb::RenameBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_id(blob_id);
    request.set_new_name(new_name);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.RenameBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.RenameBlob", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientAccess::ResizeBlob(
        const std::string &user_id, const std::string &blob_id,
        uint64_t new_size, std::string &errmsg) {

    cyprestore::access::pb::ResizeBlobRequest request;
    cyprestore::access::pb::ResizeBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_id(blob_id);
    request.set_blob_size(new_size);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.ResizeBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.ResizeBlob", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientAccess::QueryBlob(
        const std::string &user_id, const std::string &blob_id, Blob &blob,
        std::string &errmsg) {

    cyprestore::access::pb::QueryBlobRequest request;
    cyprestore::access::pb::QueryBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_id(blob_id);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.QueryBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.QueryBlob", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    blob = response.blob_info();
    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientAccess::ListBlobs(
        const std::string &user_id, const int from, const int count,
        std::list<Blob> &Blobs, std::string &errmsg) {

    cyprestore::access::pb::ListBlobsRequest request;
    cyprestore::access::pb::ListBlobsResponse response;
    request.set_user_id(user_id);
    request.set_from(from);
    request.set_count(count);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.ListBlobs(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.ListBlobs", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    Blobs.clear();
    for (int i = 0; i < response.blobs_size(); i++) {
        Blobs.push_back(response.blobs(i));
    }

    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientAccess::QuerySet(
        const std::string &user_id, SetRoute &setroute, std::string &errmsg) {

    cyprestore::access::pb::QuerySetRequest request;
    cyprestore::access::pb::QuerySetResponse response;
    request.set_user_id(user_id);

    cyprestore::access::pb::AccessService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.QuerySet(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientAccess.QuerySet", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    setroute = response.setroute();
    return (::cyprestore::common::pb::STATUS_OK);
}

}  // namespace cyprestore
