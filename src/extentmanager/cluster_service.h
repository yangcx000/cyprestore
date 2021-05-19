/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_CLUSTER_SERVICE_H_
#define CYPRESTORE_EXTENTMANAGER_CLUSTER_SERVICE_H_

#include "extentmanager/pb/cluster.pb.h"

namespace cyprestore {
namespace extentmanager {

class ClusterServiceImpl : public pb::ClusterService {
public:
    ClusterServiceImpl() = default;
    virtual ~ClusterServiceImpl() = default;

    // Pool
    virtual void CreatePool(
            google::protobuf::RpcController *cntl_base,
            const pb::CreatePoolRequest *request,
            pb::CreatePoolResponse *response, google::protobuf::Closure *done);
    virtual void InitPool(
            google::protobuf::RpcController *cntl_base,
            const pb::InitPoolRequest *request, pb::InitPoolResponse *response,
            google::protobuf::Closure *done);

    virtual void UpdatePool(
            google::protobuf::RpcController *cntl_base,
            const pb::UpdatePoolRequest *request,
            pb::UpdatePoolResponse *response, google::protobuf::Closure *done);

    virtual void DeletePool(
            google::protobuf::RpcController *cntl_base,
            const pb::DeletePoolRequest *request,
            pb::DeletePoolResponse *response, google::protobuf::Closure *done);

    virtual void QueryPool(
            google::protobuf::RpcController *cntl_base,
            const pb::QueryPoolRequest *request,
            pb::QueryPoolResponse *response, google::protobuf::Closure *done);

    virtual void ListPools(
            google::protobuf::RpcController *cntl_base,
            const pb::ListPoolsRequest *request,
            pb::ListPoolsResponse *response, google::protobuf::Closure *done);

    // Extent Server
    virtual void QueryExtentServer(
            google::protobuf::RpcController *cntl_base,
            const pb::QueryExtentServerRequest *request,
            pb::QueryExtentServerResponse *response,
            google::protobuf::Closure *done);

    virtual void ListExtentServers(
            google::protobuf::RpcController *cntl_base,
            const pb::ListExtentServersRequest *request,
            pb::ListExtentServersResponse *response,
            google::protobuf::Closure *done);

    virtual void DeleteExtentServer(
            google::protobuf::RpcController *cntl_base,
            const pb::DeleteExtentServerRequest *request,
            pb::DeleteExtentServerResponse *response,
            google::protobuf::Closure *done);

    // Replication Group
    virtual void QueryReplicationGroup(
            google::protobuf::RpcController *cntl_base,
            const pb::QueryReplicationGroupRequest *request,
            pb::QueryReplicationGroupResponse *response,
            google::protobuf::Closure *done);

    virtual void ListReplicationGroups(
            google::protobuf::RpcController *cntl_base,
            const pb::ListReplicationGroupsRequest *request,
            pb::ListReplicationGroupsResponse *response,
            google::protobuf::Closure *done);
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_CLUSTER_SERVICE_H_
