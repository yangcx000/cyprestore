/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "heartbeat_reporter.h"

#include <brpc/channel.h>

#include "common/pb/types.pb.h"
#include "extentmanager/pb/heartbeat.pb.h"
#include "extentserver.h"
#include "utils/timer_thread.h"

namespace cyprestore {
namespace extentserver {

void HeartbeatReporter::HeartbeatEntry(void *arg) {
    HeartbeatReporter *hr = reinterpret_cast<HeartbeatReporter *>(arg);
    hr->DoHeartbeatReport();
}

void HeartbeatReporter::DoHeartbeatReport() {
    brpc::Controller cntl;
    extentmanager::pb::HeartbeatService_Stub stub(es_->em_channel_);
    extentmanager::pb::ReportHeartbeatRequest request;
    extentmanager::pb::ReportHeartbeatResponse response;

    request.mutable_es()->set_id(es_->instance_id_);
    request.mutable_es()->set_name(es_->instance_name_);
    request.mutable_es()->set_size(es_->storage_engine_->UsedSize());
    request.mutable_es()->set_capacity(es_->storage_engine_->Capacity());
    request.mutable_es()->set_pool_id(es_->pool_id_);
    request.mutable_es()->set_host(es_->host_);
    request.mutable_es()->set_rack(es_->rack_);
    // TODO(yangchunxin3): 替换status实际值
    request.mutable_es()->set_status(common::pb::ESStatus::ES_STATUS_OK);

    common::Config &config = GlobalConfig();
    auto endpoint = request.mutable_es()->mutable_endpoint();
    endpoint->set_public_ip(config.network().public_ip);
    endpoint->set_public_port(config.network().public_port);
    endpoint->set_private_ip(config.network().private_ip);
    endpoint->set_private_port(config.network().private_port);

    stub.ReportHeartbeat(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Couldn't send heartbeat to extentmanager, "
                   << cntl.ErrorText();
        return;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Couldn't report heartbeat to extentmanager, "
                   << response.status().message();
        return;
    }

    if (!once_) {
        es_->storage_engine_->SetExtentSize(response.extent_size());
        es_->SetEsReady();
        once_ = true;
        LOG(INFO) << "Receive heartbeat response, extent_size:"
                  << response.extent_size();
    }
}

Status HeartbeatReporter::Start() {
    utils::TimerThread *tt = utils::GetOrCreateGlobalTimerThread();
    if (!tt) {
        return Status(
                common::CYPRE_ER_OUT_OF_MEMORY, "couldn't create timer thread");
    }

    utils::TimerOptions to(
            true, heartbeat_interval_sec_ * 1000,
            HeartbeatReporter::HeartbeatEntry, this);
    if (tt->create_timer(to) != 0) {
        return Status(common::CYPRE_ER_OUT_OF_MEMORY, "couldn't create timer");
    }
    return Status();
}

}  // namespace extentserver
}  // namespace cyprestore
