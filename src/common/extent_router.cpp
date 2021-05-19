/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "extent_router.h"

#include <butil/logging.h>

#include "extentmanager/pb/router.pb.h"

namespace cyprestore {
namespace common {

thread_local ExtentRouterMap ExtentRouterMgr::tls_extent_router_map_;

ExtentRouterPtr ExtentRouterMgr::QueryRouter(const std::string &extent_id) {
    if (use_thread_local_) {
        auto it = tls_extent_router_map_.find(extent_id);
        if (it != tls_extent_router_map_.end()) return it->second;
    } else {
        ReadLock lock(rwlock_);
        auto it = extent_router_map_.find(extent_id);
        if (it != extent_router_map_.end()) return it->second;
    }

    auto router = queryFromRemote(extent_id);
    if (!router) {
        LOG(ERROR) << "Couldn't query extent router from remote"
                   << ", extent_id:" << extent_id;
        return nullptr;
    }

    if (use_thread_local_) {
        tls_extent_router_map_.insert(std::make_pair(extent_id, router));
    } else {
        WriteLock lock(rwlock_);
        extent_router_map_.insert(std::make_pair(extent_id, router));
    }

    return router;
}

void ExtentRouterMgr::DeleteRouter(const std::string &extent_id) {
    if (use_thread_local_) {
        tls_extent_router_map_.erase(extent_id);
    } else {
        WriteLock lock(rwlock_);
        extent_router_map_.erase(extent_id);
    }
}

void ExtentRouterMgr::Clear() {
    if (use_thread_local_) {
        tls_extent_router_map_.clear();
    } else {
        WriteLock lock(rwlock_);
        extent_router_map_.clear();
    }
}

ExtentRouterPtr ExtentRouterMgr::queryFromRemote(const std::string &extent_id) {
    extentmanager::pb::RouterService_Stub stub(em_channel_);
    brpc::Controller cntl;
    extentmanager::pb::QueryRouterRequest req;
    extentmanager::pb::QueryRouterResponse resp;

    req.set_extent_id(extent_id);
    stub.QueryRouter(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Couldn't send query router, " << cntl.ErrorText()
                   << ", extent_id:" << extent_id;
        return nullptr;
    } else if (resp.status().code() != 0) {
        LOG(ERROR) << "Couldn't query router, " << resp.status().message()
                   << ", extent_id:" << extent_id;
        return nullptr;
    }

    ExtentRouterPtr router = std::make_shared<ExtentRouter>();
    if (resp.has_router()) {
        assert((extent_id == resp.router().extent_id())
               && "router extent_id inconsistency");
        router->extent_id = extent_id;
        router->primary = toESInstance(resp.router().primary());
        for (int i = 0; i < resp.router().secondaries_size(); ++i) {
            router->secondaries.push_back(
                    toESInstance(resp.router().secondaries(i)));
        }
    } else {
        LOG(ERROR) << "Query router but empty"
                   << ", extent_id:" << extent_id;
        return nullptr;
    }

    LOG(DEBUG) << "Query router finished"
              << ", extent_id:" << extent_id;
    return router;
}

ESInstance toESInstance(const common::pb::EsInstance &pb_es) {
    ESInstance es;
    es.es_id = pb_es.es_id();
    es.public_ip = pb_es.public_ip();
    es.public_port = pb_es.public_port();
    es.private_ip = pb_es.private_ip();
    es.private_port = pb_es.private_port();
    return es;
}

}  // namespace common
}  // namespace cyprestore
