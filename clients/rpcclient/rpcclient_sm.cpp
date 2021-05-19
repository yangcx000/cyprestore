/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#include "rpcclient_sm.h"

#include "setmanager/pb/heartbeat.pb.h"
#include "setmanager/pb/set.pb.h"

namespace cyprestore {

RpcClientSM::RpcClientSM() {}

RpcClientSM::~RpcClientSM() {}

StatusCode
RpcClientSM::List(std::list<SetRoutePtr> &sets, std::string &errmsg) {
    ::cyprestore::setmanager::pb::ListSetsRequest request;
    ::cyprestore::setmanager::pb::ListSetsResponse response;

    cyprestore::setmanager::pb::SetService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.List(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientSM.ListSet", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    sets.clear();
    ::cyprestore::common::pb::SetRoute setroute;
    for (int i = 0; i < response.sets_size(); i++) {
        setroute = response.sets(i);
        SetRoutePtr setrouteptr = std::make_shared<SetRoute>(setroute);
        sets.push_back(setrouteptr);
    }

    return (STATUS_OK);
}

StatusCode RpcClientSM::Allocate(
        const string &user_name, PoolType pool_type, SetRoute &outset,
        std::string &errmsg) {
    ::cyprestore::setmanager::pb::AllocateSetRequest request;
    ::cyprestore::setmanager::pb::AllocateSetResponse response;
    request.set_user_name(user_name);
    request.set_pool_type(pool_type);

    cyprestore::setmanager::pb::SetService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.Allocate(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientSM.AllocateSet", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    outset = response.set();
    return (STATUS_OK);
}

StatusCode RpcClientSM::Query(
        const string &user_id, SetRoute &outset, std::string &errmsg) {
    ::cyprestore::setmanager::pb::QuerySetRequest request;
    ::cyprestore::setmanager::pb::QuerySetResponse response;
    request.set_user_id(user_id);

    cyprestore::setmanager::pb::SetService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.Query(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientSM.QuerySet", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    outset = response.set();
    return (STATUS_OK);
}

StatusCode RpcClientSM::ReportHeartbeat(
        const cyprestore::common::pb::Set &set, std::string &errmsg) {
    ::cyprestore::setmanager::pb::HeartbeatRequest request;
    ::cyprestore::setmanager::pb::HeartbeatResponse response;

    cyprestore::setmanager::pb::HeartbeatService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.ReportHeartbeat(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientSM.ReportHeartbeat", &cntl, response.status().code(),
            errmsg);

    return (ret);
}

}  // namespace cyprestore
