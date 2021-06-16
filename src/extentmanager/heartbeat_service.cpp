/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "heartbeat_service.h"

#include <brpc/server.h>
#include <butil/logging.h>

#include "common/error_code.h"
#include "common/pb/types.pb.h"
#include "extent_manager.h"

namespace cyprestore {
namespace extentmanager {

// TODO: when recevie no heartbeat from es for a period of time, we should check
// or failover this es

void HeartbeatServiceImpl::ReportHeartbeat(
        google::protobuf::RpcController *cntl_base,
        const pb::ReportHeartbeatRequest *request,
        pb::ReportHeartbeatResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // TODO check param
    if (request->es().name().empty() || request->es().pool_id().empty()) {
        LOG(ERROR) << "es name or pool id empty, name:" << request->es().name()
                   << ", pool_id:" << request->es().pool_id();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("es name or pool id empty");
        return;
    }

    bool exist = ExtentManager::GlobalInstance().get_pool_mgr()->pool_valid(
            request->es().pool_id());
    if (!exist) {
        LOG(ERROR) << "Can't find pool, pool_id:" << request->es().pool_id();
        response->mutable_status()->set_code(common::CYPRE_EM_POOL_NOT_FOUND);
        response->mutable_status()->set_message("not found pool");
        return;
    }

    exist = ExtentManager::GlobalInstance().get_pool_mgr()->es_exist(
            request->es().pool_id(), request->es().id());
    if (!exist) {
        auto status = ExtentManager::GlobalInstance().get_pool_mgr()->create_es(
                request->es().id(), request->es().name(),
                request->es().endpoint().public_ip(),
                request->es().endpoint().public_port(),
                request->es().endpoint().private_ip(),
                request->es().endpoint().private_port(), request->es().size(),
                request->es().capacity(), request->es().pool_id(),
                request->es().host(), request->es().rack());

        if (!status.ok()) {
            LOG(ERROR) << "Receive new es : " << request->es().name()
                       << " heartbeat, but create es info failed. Status: "
                       << status.ToString();
            response->mutable_status()->set_code(status.code());
            response->mutable_status()->set_message(status.ToString());
            return;
        }
    } else {
        auto status = ExtentManager::GlobalInstance().get_pool_mgr()->update_es(
                request->es().pool_id(), request->es().id(),
                request->es().name(), request->es().endpoint().public_ip(),
                request->es().endpoint().public_port(),
                request->es().endpoint().private_ip(),
                request->es().endpoint().private_port(), request->es().size());
        if (!status.ok()) {
            LOG(ERROR) << "Receive es : " << request->es().name()
                       << " heartbeat, but update es info failed. Status: "
                       << status.ToString();
            response->mutable_status()->set_code(status.code());
            response->mutable_status()->set_message(status.ToString());
            return;
        }
    }

    response->mutable_status()->set_code(common::CYPRE_OK);
    response->set_extent_size(GlobalConfig().extentmanager().extent_size);
    int router_version =
            ExtentManager::GlobalInstance().get_pool_mgr()->get_router_version(
                    request->es().pool_id());
    response->set_router_version(router_version);

    LOG(INFO) << "Receive heartbeat from es, es_id:" << request->es().id();
    DescribeExtentServer(request->es());
}

void HeartbeatServiceImpl::DescribeExtentServer(
        const common::pb::ExtentServer &es) {
    /*
    LOG(INFO) << "\n**********Describe ExtentServer**********"
              << "\nid:" << es.id()
              << "\nname:" << es.name()
              << "\npublic_ip:" << es.public_ip()
              << "\npublic_port:" << es.public_port()
              << "\nprivate_ip:" << es.private_ip()
              << "\nprivate_port:" << es.private_port()
              << "\nsize:" << es.size()
              << "\ncapacity:" << es.capacity()
              << "\npool_id:" << es.pool_id()
              << "\nhost:" << es.host()
              << "\nrack:" << es.rack()
              << "\nstatus:" << es.status()
              << "\ncreate_date:" << es.create_date()
              << "\nupdate_date:" << es.update_date();
    */
}

}  // namespace extentmanager
}  // namespace cyprestore
