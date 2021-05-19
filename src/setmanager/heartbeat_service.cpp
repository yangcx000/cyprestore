/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "heartbeat_service.h"

#include <brpc/server.h>
#include <butil/logging.h>

#include <iomanip>  // setfill/setw

#include "common/error_code.h"
#include "setmanager.h"

namespace cyprestore {
namespace setmanager {

void HeartbeatServiceImpl::ReportHeartbeat(
        google::protobuf::RpcController *cntl_base,
        const pb::HeartbeatRequest *request, pb::HeartbeatResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    SetManager &setmanager = SetManager::GlobalInstance();
    setmanager.AddSet(request->set());
    if (GlobalConfig().DebugLog()) {
        PrintSet(request->set());
    }
    response->mutable_status()->set_code(common::CYPRE_OK);
    LOG(DEBUG) << "Receive heartbeat from set " << request->set().id();
}

void HeartbeatServiceImpl::PrintSet(const common::pb::Set &set) {
    LOG(DEBUG) << "\n"
               << std::setfill('-') << std::setw(40) << '-' << "[Set Info]"
               << std::setfill('-') << std::setw(40) << '-'
               << "\nset_id:" << set.id() << "\nset_name:" << set.name()
               << "\nem_id:" << set.em().id() << "\nem_name:" << set.em().name()
               << "\nem_ip:" << set.em().endpoint().public_ip()
               << "\nem_port:" << set.em().endpoint().public_port();

    LOG(DEBUG) << "\n"
               << std::setfill('-') << std::setw(40) << '-' << "[Pool Info]"
               << std::setfill('-') << std::setw(40) << '-';
    for (int i = 0; i < set.pools_size(); i++) {
        LOG(DEBUG) << "\npool_id:" << set.pools(i).id()
                   << "\npool_name:" << set.pools(i).name()
                   << "\npool_type:" << set.pools(i).type()
                   << "\npool_status:" << set.pools(i).status()
                   << "\nreplica_count:" << set.pools(i).replica_count()
                   << "\nrg_count:" << set.pools(i).rg_count()
                   << "\nes_count:" << set.pools(i).es_count()
                   << "\nfailure_domain:" << set.pools(i).failure_domain()
                   << "\ncreate_date::" << set.pools(i).create_date()
                   << "\nupdate_date::" << set.pools(i).update_date()
                   << "\ncapacity::" << set.pools(i).capacity()
                   << "\nsize::" << set.pools(i).size()
                   << "\nnum_extents::" << set.pools(i).num_extents();
    }
}

}  // namespace setmanager
}  // namespace cyprestore
