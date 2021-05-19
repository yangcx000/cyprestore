/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "router_service.h"

#include <brpc/server.h>
#include <butil/logging.h>

#include "common/error_code.h"
#include "common/extent_id_generator.h"
#include "common/pb/types.pb.h"
#include "extent_manager.h"

namespace cyprestore {
namespace extentmanager {

void RouterServiceImpl::QueryRouter(
        google::protobuf::RpcController *cntl_base,
        const pb::QueryRouterRequest *request,
        pb::QueryRouterResponse *response, google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    // 1. check param
    if (request->extent_id().empty()) {
        LOG(ERROR) << "Query router failed, extent id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("extentid empty");
        return;
    }

    // 2. query rg_id & pool_id
    std::string rg_id;
    std::string pool_id;
    auto status =
            ExtentManager::GlobalInstance().get_router_mgr()->query_router(
                    request->extent_id(), &pool_id, &rg_id);
    if (!status.ok()) {
        LOG(ERROR) << "Query router failed, status: " << status.ToString();
        response->mutable_status()->set_code(status.code());
        response->mutable_status()->set_message(status.ToString());
        return;
    }

    // 3. check pool or blob valid
    std::string blob_id =
            common::ExtentIDGenerator::GetBlobId(request->extent_id());
    if (!ExtentManager::GlobalInstance().get_pool_mgr()->pool_valid(pool_id)
        || !ExtentManager::GlobalInstance().get_pool_mgr()->blob_valid(
                pool_id, blob_id)) {
        LOG(ERROR) << "Query router failed, pool or blob not valid, may have "
                      "be deleted";
        response->mutable_status()->set_code(common::CYPRE_EM_BLOB_NOT_FOUND);
        response->mutable_status()->set_message("extent router not found");
        return;
    }

    // 4. query es ids through rg_id
    auto es_group =
            ExtentManager::GlobalInstance().get_pool_mgr()->query_es_group(
                    pool_id, rg_id);

    // 5. query es router through extent server ids
    common::pb::ExtentRouter router;
    router.set_extent_id(request->extent_id());
    router.set_rg_id(rg_id);
    ExtentManager::GlobalInstance().get_pool_mgr()->query_es_router(
            pool_id, es_group, &router);
    uint32_t router_version =
            ExtentManager::GlobalInstance().get_pool_mgr()->get_router_version(
                    pool_id);
    response->set_router_version(router_version);
    *response->mutable_router() = router;
    response->mutable_status()->set_code(common::CYPRE_OK);
}

}  // namespace extentmanager
}  // namespace cyprestore
