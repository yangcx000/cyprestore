/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "set_manager.h"

#include "common/config.h"
#include "common/error_code.h"
#include "setmanager/pb/set.pb.h"
#include "utils/timer_thread.h"

namespace cyprestore {
namespace access {

void SetMgr::UpdateSetsFunc(void *arg) {
    SetMgr *sm = reinterpret_cast<SetMgr *>(arg);
    sm->UpdateSets();
}

void SetMgr::UpdateSets() {
    brpc::Controller cntl;
    setmanager::pb::ListSetsRequest request;
    setmanager::pb::ListSetsResponse response;
    setmanager::pb::SetService_Stub stub(set_channel_.get());

    stub.List(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send ListSets request failed: " << cntl.ErrorText();
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "List sets failed: " << response.status().message();
    }

    boost::unique_lock<boost::shared_mutex> lock(set_mutex_);
    set_map_.clear();
    for (int i = 0; i < response.sets_size(); i++) {
        common::pb::SetRoute setroute = response.sets(i);
        auto route = std::make_shared<common::pb::SetRoute>(setroute);
        set_map_[setroute.set_id()] = route;
    }
}

int SetMgr::Init() {
    set_channel_.reset(new brpc::Channel());
    std::string set_ip = GlobalConfig().setmanager().set_ip;
    int set_port = GlobalConfig().setmanager().set_port;
    if (set_ip.empty() || set_port == 0) {
        LOG(ERROR) << "setmanager ip or port is not valid";
        return -1;
    }
    std::stringstream ss(std::ios_base::app | std::ios_base::out);
    ss << set_ip << ":" << set_port;
    if (set_channel_->Init(ss.str().c_str(), NULL) != 0) {
        LOG(ERROR) << "Couldn't connect setmanager: " << ss.str();
        return -1;
    }

    // init a timer thread to update set info
    utils::TimerThread *tt = utils::GetOrCreateGlobalTimerThread();
    if (!tt) {
        LOG(ERROR) << "Couldn't create UpdateSets timer";
        return -1;
    }
    utils::TimerOptions to(
            true, GlobalConfig().access().update_interval_sec * 1000,
            SetMgr::UpdateSetsFunc, this);
    if (tt->create_timer(to) != 0) {
        LOG(ERROR) << "Couldn't create UpdateSets timer";
        return -1;
    }
    return 0;
}

Status SetMgr::QuerySetById(int set_id, common::pb::SetRoute *setroute) {
    Status status;
    boost::shared_lock<boost::shared_mutex> lock(set_mutex_);
    auto iter = set_map_.find(set_id);
    if (iter != set_map_.end()) {
        *setroute = *(iter->second.get());
        status.set_code(common::CYPRE_OK);
        return status;
    }
    status.set_code(common::CYPRE_AC_SET_NOT_FOUND);
    status.set_message("set not found");
    return status;
}

Status
SetMgr::QuerySet(const std::string &user_id, common::pb::SetRoute *setroute) {
    brpc::Controller cntl;
    setmanager::pb::QuerySetRequest request;
    setmanager::pb::QuerySetResponse response;
    setmanager::pb::SetService_Stub stub(set_channel_.get());

    request.set_user_id(user_id);
    Status status;
    stub.Query(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send query set request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Query set failed: " << response.status().message();
        return response.status();
    }

    *setroute = response.set();
    status.set_code(common::CYPRE_OK);
    return status;
}

Status SetMgr::AllocateSet(
        const std::string user_name, common::pb::PoolType type,
        common::pb::SetRoute *setroute) {
    brpc::Controller cntl;
    setmanager::pb::AllocateSetRequest request;
    setmanager::pb::AllocateSetResponse response;
    setmanager::pb::SetService_Stub stub(set_channel_.get());

    Status status;
    request.set_pool_type(type);
    request.set_user_name(user_name);
    stub.Allocate(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Send allocate set request failed: " << cntl.ErrorText();
        status.set_code(common::CYPRE_ER_NET_ERROR);
        status.set_message(cntl.ErrorText());
        return status;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Allocate set failed: " << response.status().message();
        return response.status();
    }

    *setroute = response.set();
    status.set_code(common::CYPRE_OK);
    return status;
}

}  // namespace access
}  // namespace cyprestore
