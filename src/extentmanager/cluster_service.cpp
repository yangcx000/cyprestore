/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "cluster_service.h"

#include <brpc/server.h>
#include <butil/logging.h>

#include <string>

#include "common/error_code.h"
#include "common/pb/types.pb.h"
#include "extent_manager.h"
#include "pool.h"
#include "utils/pb_transfer.h"

namespace cyprestore {
namespace extentmanager {

void ClusterServiceImpl::CreatePool(
        google::protobuf::RpcController *cntl_base,
        const pb::CreatePoolRequest *request, pb::CreatePoolResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    // 1. check param
    if (request->name().empty() || !utils::pool_type_valid(request->type())
        || !utils::failure_domain_valid(request->failure_domain())
        || request->es_count() < 1
        || !replica_count_valid(request->replica_count())
        || request->replica_count() > request->es_count()) {
        LOG(ERROR) << "Poolname empty or pool_type invalid or "
                   << "failure_domain invalid or replica_count invalid"
                   << ", pool_name:" << request->name()
                   << ", type:" << request->type()
                   << ", failure_domain:" << request->failure_domain()
                   << ", es_count:" << request->es_count()
                   << ", replica_count:" << request->replica_count();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("argument invalid");
        return;
    }

    // 2. create pool
    PoolType type = utils::FromPbPoolType(request->type());
    PoolFailureDomain fd =
            utils::FromPbFailureDomain(request->failure_domain());
    std::string pool_id;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->create_pool(
            request->name(), type, fd, request->replica_count(),
            request->es_count(), &pool_id);

    if (!status.ok()) {
        LOG(ERROR) << "Create pool failed, pool_name:" << request->name();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    response->set_pool_id(pool_id);
    LOG(INFO) << "Create pool succeed"
              << ", pool_name:" << request->name() << ", pool_id:" << pool_id;
}

void ClusterServiceImpl::UpdatePool(
        google::protobuf::RpcController *cntl_base,
        const pb::UpdatePoolRequest *request, pb::UpdatePoolResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    // 1. check param
    if (request->pool_id().empty()) {
        LOG(ERROR) << "Pool id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("argument invalid");
        return;
    }
    std::string new_name = std::string();
    PoolType type = kPoolTypeUnknown;
    PoolFailureDomain fd = kFailureDomainUnknown;
    int replica_count = 0;
    int es_count = 0;
    bool valid = true;
    if (request->has_name()) {
        new_name = request->name();
    }
    if (request->has_type()) {
        if (utils::pool_type_valid(request->type())) {
            type = utils::FromPbPoolType(request->type());
        } else {
            valid = false;
        }
    }
    if (request->has_replica_count()) {
        if (replica_count_valid(request->replica_count())) {
            replica_count = request->replica_count();
        } else {
            valid = false;
        }
    }
    if (request->has_es_count()) {
        if (request->es_count() > 0) {
            es_count = request->es_count();
        } else {
            valid = false;
        }
    }
    if (request->has_failure_domain()) {
        if (utils::failure_domain_valid(request->failure_domain())) {
            fd = utils::FromPbFailureDomain(request->failure_domain());
        } else {
            valid = false;
        }
    }
    if (!valid) {
        LOG(ERROR) << "Argument invalid";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("argument invalid");
        return;
    }

    // 2. update pool
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->update_pool(
            request->pool_id(), new_name, type, fd, replica_count, es_count);

    if (!status.ok()) {
        LOG(ERROR) << "Update pool failed, pool_id:" << request->pool_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Update pool succeed, pool_id:" << request->pool_id();
}

void ClusterServiceImpl::DeletePool(
        google::protobuf::RpcController *cntl_base,
        const pb::DeletePoolRequest *request, pb::DeletePoolResponse *response,
        google::protobuf::Closure *done) {
    // 1. check param
    brpc::ClosureGuard done_guard(done);
    if (request->pool_id().empty()) {
        LOG(ERROR) << "Poolid empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("pool id empty");
        return;
    }

    // 2. delete pool
    auto status =
            ExtentManager::GlobalInstance().get_pool_mgr()->soft_delete_pool(
                    request->pool_id());
    if (!status.ok()) {
        LOG(ERROR) << "Delete pool failed, pool_id:" << request->pool_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Delete pool succeed, pool_id:" << request->pool_id();
}

void ClusterServiceImpl::InitPool(
        google::protobuf::RpcController *cntl_base,
        const pb::InitPoolRequest *request, pb::InitPoolResponse *response,
        google::protobuf::Closure *done) {
    // 1. check param
    brpc::ClosureGuard done_guard(done);
    if (request->pool_id().empty()) {
        LOG(ERROR) << "Poolid empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("poolid empty");
        return;
    }

    // 2. init pool
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->init_pool(
            request->pool_id());
    if (!status.ok()) {
        LOG(ERROR) << "Init pool failed, pool_id:" << request->pool_id();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Init pool succeed, pool_id:" << request->pool_id();
}

void ClusterServiceImpl::QueryPool(
        google::protobuf::RpcController *cntl_base,
        const pb::QueryPoolRequest *request, pb::QueryPoolResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1 check param
    if (request->pool_id().empty()) {
        LOG(ERROR) << "Pool id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("pool id empty");
        return;
    }

    // 2 query pool
    common::pb::Pool pool;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->query_pool(
            request->pool_id(), &pool);
    if (!status.ok()) {
        LOG(ERROR) << "Rename pool failed";
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    *response->mutable_pool() = pool;
    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Query pool succeed, pool_id:" << request->pool_id();
}

void ClusterServiceImpl::ListPools(
        google::protobuf::RpcController *cntl_base,
        const pb::ListPoolsRequest *request, pb::ListPoolsResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    std::vector<common::pb::Pool> pools;
    auto status =
            ExtentManager::GlobalInstance().get_pool_mgr()->list_pools(&pools);
    for (uint32_t i = 0; i < pools.size(); i++) {
        *response->add_pools() = pools[i];
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "List pools succeed, nr_pools:" << pools.size();
}

void ClusterServiceImpl::QueryExtentServer(
        google::protobuf::RpcController *cntl_base,
        const pb::QueryExtentServerRequest *request,
        pb::QueryExtentServerResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    // 1 check param
    if (request->pool_id().empty()) {
        LOG(ERROR) << "pool_id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("pool_id empty");
        return;
    }

    // 2 query es
    common::pb::ExtentServer es;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->query_es(
            request->pool_id(), request->es_id(), &es);
    if (!status.ok()) {
        LOG(ERROR) << "Pool or es not found";
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    *response->mutable_es() = es;
    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Query extent server succeed, es_id:" << request->es_id();
}

void ClusterServiceImpl::DeleteExtentServer(
        google::protobuf::RpcController *cntl_base,
        const pb::DeleteExtentServerRequest *request,
        pb::DeleteExtentServerResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    // 1 check param
    if (request->pool_id().empty()) {
        LOG(ERROR) << "pool_id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("pool_id empty");
        return;
    }

    // 2 delete es
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->delete_es(
            request->pool_id(), request->es_id());
    if (!status.ok()) {
        LOG(ERROR) << "Delete es failed";
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Delete extent server succeed, es_id:" << request->es_id();
}

void ClusterServiceImpl::ListExtentServers(
        google::protobuf::RpcController *cntl_base,
        const pb::ListExtentServersRequest *request,
        pb::ListExtentServersResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    // 1 check param
    if (request->pool_id().empty()) {
        LOG(ERROR) << "pool_id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("pool_id empty");
        return;
    }

    // 2 check pool
    auto exist = ExtentManager::GlobalInstance().get_pool_mgr()->pool_valid(
            request->pool_id());
    if (!exist) {
        LOG(ERROR) << "Pool not found";
        response->mutable_status()->set_code(common::CYPRE_EM_POOL_NOT_FOUND);
        response->mutable_status()->set_message("pool not found");
        return;
    }

    // 3 query es
    std::vector<common::pb::ExtentServer> ess;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->list_es(
            request->pool_id(), &ess);
    for (uint32_t i = 0; i < ess.size(); i++) {
        *response->add_ess() = ess[i];
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "List ess succeed, nr_ess:" << ess.size();
}

// Replication Group
void ClusterServiceImpl::QueryReplicationGroup(
        google::protobuf::RpcController *cntl_base,
        const pb::QueryReplicationGroupRequest *request,
        pb::QueryReplicationGroupResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    // 1. check param
    if (request->pool_id().empty() || request->rg_id().empty()) {
        LOG(ERROR) << "pool_id or rg_id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("pool_id or rg_id empty");
        return;
    }

    // 2. query rg
    common::pb::ReplicaGroup rg;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->query_rg(
            request->pool_id(), request->rg_id(), &rg);
    if (!status.ok()) {
        LOG(ERROR) << "Replication group not found";
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    *response->mutable_rg() = rg;
    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "Query replication group succeed, rg_id:" << request->rg_id();
}

void ClusterServiceImpl::ListReplicationGroups(
        google::protobuf::RpcController *cntl_base,
        const pb::ListReplicationGroupsRequest *request,
        pb::ListReplicationGroupsResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    // 1. check param
    if (request->pool_id().empty()) {
        LOG(ERROR) << "pool_id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("pool_id empty");
        return;
    }

    // 2. list replication groups
    std::vector<common::pb::ReplicaGroup> rgs;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->list_rg(
            request->pool_id(), &rgs);
    for (uint32_t i = 0; i < rgs.size(); i++) {
        *response->add_rgs() = rgs[i];
    }

    response->mutable_status()->set_code(status.code());
    LOG(INFO) << "List rgs succeed, nr_rgs:" << rgs.size();
}

}  // namespace extentmanager
}  // namespace cyprestore
