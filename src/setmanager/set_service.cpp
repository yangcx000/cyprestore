/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "set_service.h"

#include <brpc/server.h>
#include <butil/logging.h>

#include "common/error_code.h"
#include "common/pb/types.pb.h"
#include "extentmanager/pb/resource.pb.h"
#include "utils/pb_transfer.h"

namespace cyprestore {
namespace setmanager {

void SetServiceImpl::List(
        google::protobuf::RpcController *cntl_base,
        const pb::ListSetsRequest *request, pb::ListSetsResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    SetManager &setmanager = SetManager::GlobalInstance();
    if (!setmanager.Available()) {
        LOG(ERROR) << "Setmanager not ready"
                   << ", actual_num_sets:" << setmanager.num_sets()
                   << ", expect_num_sets:"
                   << GlobalConfig().setmanager().num_sets;
        response->mutable_status()->set_code(common::CYPRE_SM_NOT_READY);
        response->mutable_status()->set_message(
                "Setmanager not ready, please retry");
        return;
    }

    SetArray sets = setmanager.ListSets();
    for (auto it = sets.begin(); it != sets.end(); ++it) {
        auto set_route = response->add_sets();
        *set_route = utils::ToPbSetRoute((*it).pb_set);
    }

    response->mutable_status()->set_code(common::CYPRE_OK);
    LOG(DEBUG) << "List sets, num_sets:" << sets.size();
}

void SetServiceImpl::Allocate(
        google::protobuf::RpcController *cntl_base,
        const pb::AllocateSetRequest *request,
        pb::AllocateSetResponse *response, google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    SetManager &setmanager = SetManager::GlobalInstance();
    if (!setmanager.Available()) {
        LOG(ERROR) << "Setmanager not ready"
                   << ", actual_num_sets:" << setmanager.num_sets()
                   << ", expect_num_sets:"
                   << GlobalConfig().setmanager().num_sets;
        response->mutable_status()->set_code(common::CYPRE_SM_NOT_READY);
        response->mutable_status()->set_message(
                "Setmanager not ready, please retry");
        return;
    }

    common::pb::PoolType pool_type = common::pb::PoolType::POOL_TYPE_HDD;
    if (request->has_pool_type()) pool_type = request->pool_type();

    if (pool_type == common::pb::PoolType::POOL_TYPE_UNKNOWN) {
        LOG(ERROR) << "Unknown pool type"
                   << ", pool_type:" << static_cast<int>(pool_type)
                   << ", username:" << request->user_name();

        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("Unknown pool type");
        return;
    }

    Set set = setmanager.AllocateSet(pool_type);
    if (set.empty()) {
        LOG(ERROR) << "Couldn't allocate set for " << request->user_name()
                   << ", pool_type:" << static_cast<int>(pool_type);

        response->mutable_status()->set_code(
                common::CYPRE_SM_RESOURCE_EXHAUSTED);
        response->mutable_status()->set_message("Couldn't allocate set");
        return;
    }

    auto set_route = response->mutable_set();
    *set_route = utils::ToPbSetRoute(set.pb_set);

    LOG(INFO) << "Allocate set succeed, username:" << request->user_name()
              << ", pool_type:" << static_cast<int>(pool_type);
    response->mutable_status()->set_code(common::CYPRE_OK);
}

void SetServiceImpl::Query(
        google::protobuf::RpcController *cntl_base,
        const pb::QuerySetRequest *request, pb::QuerySetResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    SetManager &setmanager = SetManager::GlobalInstance();
    SetArray sets = setmanager.ListSets();
    int index = BroadcastRequest(sets, request->user_id());
    if (index < 0) {
        response->mutable_status()->set_code(common::CYPRE_SM_USER_NOT_FOUND);
        response->mutable_status()->set_message("user id not found");
        return;
    }

    auto set_route = response->mutable_set();
    *set_route = utils::ToPbSetRoute(sets[index].pb_set);
    response->mutable_status()->set_code(common::CYPRE_OK);
    LOG(DEBUG) << "Query set by userid succeed"
               << ", user_id:" << request->user_id()
               << ", set_id:" << sets[index].pb_set.id();
}

int SetServiceImpl::BroadcastRequest(
        const SetArray &sets, const std::string &user_id) {
    std::vector<brpc::Controller> cntls(sets.size());
    std::vector<extentmanager::pb::QueryUserResponse> resps(sets.size());
    for (int i = 0; i < (int)sets.size(); ++i) {
        std::string addr =
                sets[i].pb_set.em().endpoint().public_ip() + ":"
                + std::to_string(sets[i].pb_set.em().endpoint().public_port());
        brpc::Channel channel;
        // XXX: add channel options
        if (channel.Init(addr.c_str(), nullptr) != 0) {
            LOG(ERROR) << "Couldn't init channel to " << addr;
            continue;
        }

        extentmanager::pb::ResourceService_Stub stub(&channel);
        extentmanager::pb::QueryUserRequest req;

        req.set_user_id(user_id);
        stub.QueryUser(&cntls[i], &req, &resps[i], brpc::DoNothing());
    }

    for (int i = 0; i < (int)sets.size(); ++i) {
        brpc::Join(cntls[i].call_id());
        if (cntls[i].Failed()) {
            LOG(ERROR) << "Failed to query user from set "
                       << sets[i].pb_set.id();
            continue;
        }

        if (resps[i].status().code() == 0) {
            return i;
        }
    }

    return -1;
}

}  // namespace setmanager
}  // namespace cyprestore
