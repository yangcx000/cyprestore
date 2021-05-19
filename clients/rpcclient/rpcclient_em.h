/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#ifndef RPCCLIENT_EM_H_
#define RPCCLIENT_EM_H_

#include "common/pb/types.pb.h"
#include "rpcclient_base.h"

typedef std::shared_ptr<User> UserPtr;
typedef std::shared_ptr<Blob> BlobPtr;
typedef std::shared_ptr<Pool> PoolPtr;
typedef std::shared_ptr<ExtentServer> ExtentServerPtr;
typedef std::shared_ptr<ReplicaGroup> ReplicaGroupPtr;

namespace cyprestore {

class RpcClientEM : public RpcClient {
public:
    RpcClientEM();
    virtual ~RpcClientEM();

    StatusCode CreatePool(
            const string &name, PoolType type, int replica_count, int rg_count,
            int es_count, FailureDomain failure_domain, string &pool_id,
            std::string &errmsg);
    StatusCode InitPool(const string &pool_id, std::string &errmsg);
    StatusCode RenamePool(
            const string &pool_id, const string &new_name, std::string &errmsg);
    StatusCode
    QueryPool(const string &pool_id, Pool &pool, std::string &errmsg);
    StatusCode ListPools(std::list<PoolPtr> &pools, std::string &errmsg);
    StatusCode QueryExtentServer(
            int es_id, const string &pool_id, ExtentServer &es,
            std::string &errmsg);
    StatusCode ListExtentServers(
            const string &pool_id, std::list<ExtentServerPtr> &ExtentServers,
            std::string &errmsg);
    StatusCode QueryReplicationGroup(
            const string &rg_id, const string &pool_id, ReplicaGroup &rg,
            std::string &errmsg);
    StatusCode ListReplicationGroups(
            const string &pool_id, std::list<ReplicaGroupPtr> &rgs,
            std::string &errmsg);
    StatusCode ReportHeartbeat(
            const cyprestore::common::pb::ExtentServer &es,
            std::string &errmsg);

    StatusCode CreateUser(
            const std::string &user_name, const std::string &email,
            const std::string &comments, const std::string &pool_id,
            std::string &user_id, std::string &errmsg);
    StatusCode DeleteUser(const std::string &user_id, std::string &errmsg);
    StatusCode RenameUser(
            const std::string &user_id, const std::string &user_name,
            std::string &errmsg);
    StatusCode
    QueryUser(const std::string &user_id, User &userinfo, std::string &errmsg);
    StatusCode ListUsers(std::list<UserPtr> &Users, std::string &errmsg);

    StatusCode CreateBlob(
            const std::string &user_id, const std::string &blob_name,
            uint64_t blob_size, const BlobType blob_type,
            const std::string &instance_id, const std::string &blob_desc,
            const std::string &pool_id, std::string &blob_id,
            std::string &errmsg);
    StatusCode DeleteBlob(
            const std::string &user_id, const std::string &blob_id,
            std::string &errmsg);
    StatusCode RenameBlob(
            const std::string &user_id, const std::string &blob_id,
            const std::string &new_name, std::string &errmsg);
    StatusCode ResizeBlob(
            const std::string &user_id, const std::string &blob_id,
            uint64_t new_size, std::string &errmsg);
    StatusCode QueryBlob(
            const std::string &user_id, const std::string &blob_id, Blob &blob,
            std::string &errmsg);
    StatusCode ListBlobs(
            const std::string &user_id, std::list<BlobPtr> &Blobs,
            std::string &errmsg);

    StatusCode QueryRouter(
            const std::string &blob_id, int extent_idx, ExtentRouter &router,
            std::string &errmsg);
    StatusCode FetchRouter(
            const std::string &extent_id, const std::string &pool_id,
            ExtentRouter &router, std::string &errmsg);

private:
};

}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENT_EM_H_
