/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#include "rpcclient_em.h"

#include <brpc/channel.h>

#include "extentmanager/pb/cluster.pb.h"
#include "extentmanager/pb/heartbeat.pb.h"
#include "extentmanager/pb/resource.pb.h"
#include "extentmanager/pb/router.pb.h"

namespace cyprestore {

RpcClientEM::RpcClientEM() {}

RpcClientEM::~RpcClientEM() {}

StatusCode RpcClientEM::CreatePool(
        const string &name, PoolType type, int replica_count, int rg_count,
        int es_count, FailureDomain failure_domain, string &pool_id,
        std::string &errmsg) {
    ::cyprestore::extentmanager::pb::CreatePoolRequest request;
    ::cyprestore::extentmanager::pb::CreatePoolResponse response;
    request.set_name(name);
    request.set_type(type);
    request.set_replica_count(replica_count);
    request.set_rg_count(rg_count);
    request.set_es_count(es_count);
    request.set_failure_domain(failure_domain);

    cyprestore::extentmanager::pb::ClusterService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.CreatePool(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.CreatePool", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    pool_id = response.pool_id();
    return (STATUS_OK);
}

StatusCode RpcClientEM::InitPool(const string &pool_id, std::string &errmsg) {
    ::cyprestore::extentmanager::pb::InitPoolRequest request;
    ::cyprestore::extentmanager::pb::InitPoolResponse response;
    request.set_pool_id(pool_id);

    cyprestore::extentmanager::pb::ClusterService_Stub stub(&channel);
    brpc::Controller cntl;

    stub.InitPool(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.InitPool", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    return (STATUS_OK);
}

StatusCode RpcClientEM::RenamePool(
        const string &pool_id, const string &new_name, std::string &errmsg) {
    ::cyprestore::extentmanager::pb::RenamePoolRequest request;
    ::cyprestore::extentmanager::pb::RenamePoolResponse response;
    request.set_pool_id(pool_id);
    request.set_new_name(new_name);

    cyprestore::extentmanager::pb::ClusterService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.RenamePool(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.RenamePool", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    return (STATUS_OK);
}

StatusCode
RpcClientEM::QueryPool(const string &pool_id, Pool &pool, std::string &errmsg) {
    ::cyprestore::extentmanager::pb::QueryPoolRequest request;
    ::cyprestore::extentmanager::pb::QueryPoolResponse response;
    request.set_pool_id(pool_id);

    cyprestore::extentmanager::pb::ClusterService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.QueryPool(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.QueryPool", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    pool = response.pool();
    return (STATUS_OK);
}

StatusCode
RpcClientEM::ListPools(std::list<PoolPtr> &pools, std::string &errmsg) {
    ::cyprestore::extentmanager::pb::ListPoolsRequest request;
    ::cyprestore::extentmanager::pb::ListPoolsResponse response;

    cyprestore::extentmanager::pb::ClusterService_Stub stub(&channel);
    brpc::Controller cntl;

    stub.ListPools(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.ListPools", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    pools.clear();
    for (int i = 0; i < response.pools_size(); i++) {
        PoolPtr poolptr = std::make_shared<Pool>(response.pools(i));
        pools.push_back(poolptr);
    }
    return (STATUS_OK);
}

// ExtentServer messages
StatusCode RpcClientEM::QueryExtentServer(
        int es_id, const string &pool_id, ExtentServer &es,
        std::string &errmsg) {
    ::cyprestore::extentmanager::pb::QueryExtentServerRequest request;
    ::cyprestore::extentmanager::pb::QueryExtentServerResponse response;
    request.set_es_id(es_id);
    request.set_pool_id(pool_id);

    cyprestore::extentmanager::pb::ClusterService_Stub stub(&channel);
    brpc::Controller cntl;

    stub.QueryExtentServer(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.QueryExtentServer", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    es = response.es();
    return (STATUS_OK);
}

StatusCode RpcClientEM::ListExtentServers(
        const string &pool_id, std::list<ExtentServerPtr> &ExtentServers,
        std::string &errmsg) {
    ::cyprestore::extentmanager::pb::ListExtentServersRequest request;
    ::cyprestore::extentmanager::pb::ListExtentServersResponse response;
    request.set_pool_id(pool_id);

    cyprestore::extentmanager::pb::ClusterService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.ListExtentServers(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.ListExtentServers", &cntl, response.status().code(),
            errmsg);
    if (ret != STATUS_OK) return (ret);

    ExtentServers.clear();
    for (int i = 0; i < response.ess_size(); i++) {
        ExtentServerPtr ExtentServerptr =
                std::make_shared<ExtentServer>(response.ess(i));
        ExtentServers.push_back(ExtentServerptr);
    }
    return (STATUS_OK);
}

// Replication Group messages
StatusCode RpcClientEM::QueryReplicationGroup(
        const string &rg_id, const string &pool_id, ReplicaGroup &rg,
        std::string &errmsg) {
    ::cyprestore::extentmanager::pb::QueryReplicationGroupRequest request;
    ::cyprestore::extentmanager::pb::QueryReplicationGroupResponse response;
    request.set_rg_id(rg_id);
    request.set_pool_id(pool_id);

    cyprestore::extentmanager::pb::ClusterService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.QueryReplicationGroup(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.QueryReplicationGroup", &cntl,
            response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    rg = response.rg();
    return (STATUS_OK);
}

StatusCode RpcClientEM::ListReplicationGroups(
        const string &pool_id, std::list<ReplicaGroupPtr> &rgs,
        std::string &errmsg) {
    ::cyprestore::extentmanager::pb::ListReplicationGroupsRequest request;
    ::cyprestore::extentmanager::pb::ListReplicationGroupsResponse response;
    request.set_pool_id(pool_id);

    cyprestore::extentmanager::pb::ClusterService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.ListReplicationGroups(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.ListReplicationGroups", &cntl,
            response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    rgs.clear();
    for (int i = 0; i < response.rgs_size(); i++) {
        ReplicaGroupPtr rgptr = std::make_shared<ReplicaGroup>(response.rgs(i));
        rgs.push_back(rgptr);
    }
    return (STATUS_OK);
}

StatusCode RpcClientEM::ReportHeartbeat(
        const cyprestore::common::pb::ExtentServer &es, std::string &errmsg) {
    ::cyprestore::extentmanager::pb::ReportHeartbeatRequest request;
    ::cyprestore::extentmanager::pb::ReportHeartbeatResponse response;

    cyprestore::extentmanager::pb::HeartbeatService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.ReportHeartbeat(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.ReportHeartbeat", &cntl, response.status().code(),
            errmsg);

    return (ret);
}

// user
StatusCode RpcClientEM::CreateUser(
        const std::string &user_name, const std::string &email,
        const std::string &comments, const std::string &pool_id,
        std::string &user_id, std::string &errmsg) {
    cyprestore::extentmanager::pb::CreateUserRequest request;
    cyprestore::extentmanager::pb::CreateUserResponse response;
    request.set_user_name(user_name);
    request.set_email(email);
    request.set_comments(comments);
    request.set_pool_id(pool_id);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.CreateUser(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.CreateUser", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    user_id = response.user_id();
    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode
RpcClientEM::DeleteUser(const std::string &user_id, std::string &errmsg) {
    cyprestore::extentmanager::pb::DeleteUserRequest request;
    cyprestore::extentmanager::pb::DeleteUserResponse response;
    request.set_user_id(user_id);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.DeleteUser(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.DeleteUser", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    return (STATUS_OK);
}

StatusCode RpcClientEM::RenameUser(
        const std::string &user_id, const std::string &user_name,
        std::string &errmsg) {
    cyprestore::extentmanager::pb::RenameUserRequest request;
    cyprestore::extentmanager::pb::RenameUserResponse response;
    request.set_user_id(user_id);
    request.set_new_name(user_name);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.RenameUser(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.RenameUser", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    return (STATUS_OK);
}

StatusCode RpcClientEM::QueryUser(
        const std::string &user_id, User &userinfo, std::string &errmsg) {

    cyprestore::extentmanager::pb::QueryUserRequest request;
    cyprestore::extentmanager::pb::QueryUserResponse response;
    request.set_user_id(user_id);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.QueryUser(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.QueryUser", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    userinfo = response.user();
    return (::cyprestore::common::pb::STATUS_OK);
}

// List Users
StatusCode
RpcClientEM::ListUsers(std::list<UserPtr> &Users, std::string &errmsg) {

    cyprestore::extentmanager::pb::ListUsersRequest request;
    cyprestore::extentmanager::pb::ListUsersResponse response;

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;

    stub.ListUsers(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.ListUsers", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    Users.clear();
    for (int i = 0; i < response.users_size(); i++) {
        Users.push_back(std::make_shared<User>(response.users(i)));
        // Users.push_back(response.users(i));
    }

    return (::cyprestore::common::pb::STATUS_OK);
}

// blob
StatusCode RpcClientEM::CreateBlob(
        const std::string &user_id, const std::string &blob_name,
        uint64_t blob_size, const BlobType blob_type,
        const std::string &instance_id, const std::string &blob_desc,
        const std::string &pool_id, std::string &blob_id, std::string &errmsg) {
    cyprestore::extentmanager::pb::CreateBlobRequest request;
    cyprestore::extentmanager::pb::CreateBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_name(blob_name);
    request.set_blob_size(blob_size);
    request.set_blob_type(blob_type);
    request.set_instance_id(instance_id);
    request.set_blob_desc(blob_desc);
    request.set_pool_id(pool_id);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.CreateBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.CreateBlob", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    blob_id = response.blob_id();
    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientEM::DeleteBlob(
        const std::string &user_id, const std::string &blob_id,
        std::string &errmsg) {

    cyprestore::extentmanager::pb::DeleteBlobRequest request;
    cyprestore::extentmanager::pb::DeleteBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_id(blob_id);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.DeleteBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.DeleteBlob", &cntl, response.status().code(), errmsg);

    return (ret);
}

StatusCode RpcClientEM::RenameBlob(
        const std::string &user_id, const std::string &blob_id,
        const std::string &new_name, std::string &errmsg) {

    cyprestore::extentmanager::pb::RenameBlobRequest request;
    cyprestore::extentmanager::pb::RenameBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_id(blob_id);
    request.set_new_name(new_name);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.RenameBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.RenameBlob", &cntl, response.status().code(), errmsg);

    return (ret);
}

StatusCode RpcClientEM::ResizeBlob(
        const std::string &user_id, const std::string &blob_id,
        uint64_t new_size, std::string &errmsg) {

    cyprestore::extentmanager::pb::ResizeBlobRequest request;
    cyprestore::extentmanager::pb::ResizeBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_id(blob_id);
    request.set_new_size(new_size);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.ResizeBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.ResizeBlob", &cntl, response.status().code(), errmsg);

    return (ret);
}

StatusCode RpcClientEM::QueryBlob(
        const std::string &user_id, const std::string &blob_id, Blob &blob,
        std::string &errmsg) {
    cyprestore::extentmanager::pb::QueryBlobRequest request;
    cyprestore::extentmanager::pb::QueryBlobResponse response;
    request.set_user_id(user_id);
    request.set_blob_id(blob_id);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.QueryBlob(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.QueryBlob", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    blob = response.blob();
    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientEM::ListBlobs(
        const std::string &user_id, std::list<BlobPtr> &Blobs,
        std::string &errmsg) {

    cyprestore::extentmanager::pb::ListBlobsRequest request;
    cyprestore::extentmanager::pb::ListBlobsResponse response;
    request.set_user_id(user_id);

    cyprestore::extentmanager::pb::ResourceService_Stub stub(&channel);
    brpc::Controller cntl;

    stub.ListBlobs(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.ListBlobs", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    Blobs.clear();
    for (int i = 0; i < response.blobs_size(); i++) {
        Blobs.push_back(std::make_shared<Blob>(response.blobs(i)));
    }
    return (::cyprestore::common::pb::STATUS_OK);
}

// route
StatusCode RpcClientEM::QueryRouter(
        const std::string &blob_id, int extent_idx, ExtentRouter &router,
        std::string &errmsg) {
    cyprestore::extentmanager::pb::QueryRouterRequest request;
    cyprestore::extentmanager::pb::QueryRouterResponse response;
    request.set_blob_id(blob_id);
    request.set_extent_idx(extent_idx);

    cyprestore::extentmanager::pb::RouterService_Stub stub(&channel);
    brpc::Controller cntl;

    stub.QueryRouter(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.QueryRouter", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    router = response.router();
    return (::cyprestore::common::pb::STATUS_OK);
}

StatusCode RpcClientEM::FetchRouter(
        const std::string &extent_id, const std::string &pool_id,
        ExtentRouter &router, std::string &errmsg) {
    cyprestore::extentmanager::pb::FetchRouterRequest request;
    cyprestore::extentmanager::pb::FetchRouterResponse response;
    request.set_extent_id(extent_id);
    request.set_pool_id(pool_id);

    cyprestore::extentmanager::pb::RouterService_Stub stub(&channel);
    brpc::Controller cntl;

    stub.FetchRouter(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientEM.FetchRouter", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);
    router = response.router();
    return (::cyprestore::common::pb::STATUS_OK);
}

}  // namespace cyprestore
