/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "gc_manager.h"

#include <butil/logging.h>

#include <ctime>

#include "common/config.h"
#include "common/error_code.h"
#include "common/extent_id_generator.h"
#include "common/log.h"
#include "extent_manager.h"
#include "utils/timer_thread.h"

namespace cyprestore {
namespace extentmanager {

int GcManager::Init() {
    conn_pool_.reset(new common::ConnectionPool());

    utils::TimerThread *tt = utils::GetOrCreateGlobalTimerThread();
    if (!tt) {
        LOG(ERROR) << "Couldn't create timer thread";
        return -1;
    }

    utils::TimerOptions to(
            true, GlobalConfig().extentmanager().gc_interval_sec * 1000,
            GcManager::GCFunc, this);
    if (tt->create_timer(to) != 0) {
        LOG(ERROR) << "Couldn't create gc timer";
        return -1;
    }
    gcing_ = false;
    return 0;
}

void GcManager::GCFunc(void *arg) {
    GcManager *gm = reinterpret_cast<GcManager *>(arg);
    gm->gc();
}

void GcManager::gc() {
    if (gcing_) {
        return;
    }
    if (!timeValid()) {
        return;
    }
    gcing_ = true;

    // 1. delete soft deleted pool
    // delete all blob meta info belong to this pool
    // delete all router meta info belong to this pool
    deletePools();

    // 2. delete blobs belong to soft deleted user
    // delete blob meta info
    // delete router meta info
    // delete user meta info
    // delete remote es space
    deleteUserBlobs();

    // 3. delete soft deleted blobs
    // delete blob meta info
    // delete router meta info
    // delete remote es space
    deleteBlobs();

    gcing_ = false;
}

void GcManager::deleteUserBlobs() {
    // list deleted user
    std::vector<std::string> users;
    ExtentManager::GlobalInstance().get_user_mgr()->list_deleted_users(&users);
    bool success = true;
    for (auto &user_id : users) {
        LOG(INFO) << "[gc mananger] Begin delete user: " << user_id << " info";
        // list pools
        std::vector<std::string> pools;
        auto status =
                ExtentManager::GlobalInstance().get_user_mgr()->list_pools(
                        user_id, &pools);
        for (auto &pool_id : pools) {
            std::vector<std::string> blobs;
            auto status =
                    ExtentManager::GlobalInstance().get_pool_mgr()->list_blobs(
                            pool_id, user_id, &blobs);
            if (!status.ok()) {
                LOG(ERROR) << "[gc manager] List blobs failed, pool_id "
                           << pool_id << ", user_id: " << user_id;
                success = false;
                continue;
            }
            status = deleteBlobs(pool_id, blobs, true);
            success = status.ok() && success;
        }
        if (!success) {
            LOG(ERROR) << "[gc manager] Delete user: " << user_id
                       << " info failed, try next time";
            continue;
        }
        ExtentManager::GlobalInstance().get_user_mgr()->delete_user(user_id);
        LOG(INFO) << "[gc manager] Delete user: " << user_id << " info succeed";
    }
}

void GcManager::deleteBlobs() {
    // list all pools
    std::vector<std::string> pools;
    ExtentManager::GlobalInstance().get_pool_mgr()->list_pools(&pools);

    // iter all pools
    for (auto &pool_id : pools) {
        // list all deleted blobs
        std::vector<std::string> blobs;
        auto status =
                ExtentManager::GlobalInstance().get_pool_mgr()->list_blobs(
                        pool_id, true, &blobs);
        if (!status.ok()) {
            LOG(ERROR) << "[gc manager] List blobs failed, pool_id " << pool_id;
            continue;
        }
        deleteBlobs(pool_id, blobs, true);
    }
}

void GcManager::deletePools() {
    std::vector<std::string> pools;
    ExtentManager::GlobalInstance().get_pool_mgr()->list_deleted_pools(&pools);

    for (auto &pool_id : pools) {
        LOG(INFO) << "[gc manager] Begin delete pool: " << pool_id << " info";
        // list all blobs
        std::vector<std::string> blobs;
        auto status =
                ExtentManager::GlobalInstance().get_pool_mgr()->list_blobs(
                        pool_id, false, &blobs);
        if (!status.ok()) {
            LOG(ERROR) << "[gc manager] List blobs failed, pool_id " << pool_id;
            continue;
        }
        // iter all deleted blobs
        status = deleteBlobs(pool_id, blobs, false);
        if (status.ok()) {
            ExtentManager::GlobalInstance().get_pool_mgr()->delete_all_es(
                    pool_id);
            ExtentManager::GlobalInstance().get_pool_mgr()->delete_all_rg(
                    pool_id);
            ExtentManager::GlobalInstance().get_pool_mgr()->delete_pool(
                    pool_id);
            LOG(INFO) << "[gc manager] Delete pool: " << pool_id << " succeed";
        } else {
            LOG(INFO) << "[gc manager] Delete pool: " << pool_id
                      << " failed, try next time";
        }
    }
}

Status GcManager::deleteBlobs(
        const std::string &pool_id, const std::vector<std::string> &blobs,
        bool delete_remote) {
    bool success = true;
    for (auto &blob_id : blobs) {
        // if time is not in gc period, return
        if (!timeValid()) {
            LOG(INFO) << "[gc manager] Delete blob exceed time limit, try next "
                         "time";
            return Status(common::CYPRE_EM_EXCEED_TIME_LIMIT);
        }
        auto status = deleteOneBlob(pool_id, blob_id, delete_remote);
        if (!status.ok()) {
            LOG(ERROR) << "[gc manager] Delete blob: " << blob_id
                       << " failed, pool_id: " << pool_id
                       << ", status: " << status.ToString();
            success = false;
            continue;
        }
        LOG(INFO) << "[gc manager] Delete blob: " << blob_id
                  << " succeed, pool_id: " << pool_id;
    }

    if (success) {
        return Status();
    }
    return Status(common::CYPRE_EM_TRY_AGAIN);
}

Status GcManager::deleteOneBlob(
        const std::string &pool_id, const std::string &blob_id,
        bool delete_remote) {
    LOG(INFO) << "[gc manager] Begin delete blob: " << blob_id;
    common::pb::Blob blob;
    auto status = ExtentManager::GlobalInstance().get_pool_mgr()->query_blob(
            blob_id, true, &blob);
    if (!status.ok()) {
        return status;
    }
    uint64_t size = blob.size();
    uint64_t extent_num = size / GlobalConfig().extentmanager().extent_size;
    for (uint64_t index = 1; index <= extent_num; index++) {
        auto id = common::ExtentIDGenerator::GenerateExtentID(blob_id, index);
        if (delete_remote) {
            status = deleteRemoteExtent(id);
            if (!status.ok()) {
                LOG(ERROR) << "[gc manager] Delete remote extent failed, "
                              "extent_id: "
                           << id;
                return status;
            }
        }

        // delete router info
        status =
                ExtentManager::GlobalInstance().get_router_mgr()->delete_router(
                        id);
        if (!status.ok()) {
            LOG(ERROR) << "[gc manager] Delete extent router: " << id
                       << " failed";
            return status;
        }
    }
    return ExtentManager::GlobalInstance().get_pool_mgr()->delete_blob(
            pool_id, blob_id);
}

Status GcManager::deleteRemoteExtent(const std::string &extent_id) {
    common::pb::ExtentRouter router;
    auto status = getRouter(extent_id, &router);
    if (!status.ok()) {
        return status;
    }
    for (uint64_t offset = 0;
         offset < GlobalConfig().extentmanager().extent_size;
         offset += block_size_) {
        status = deleteOneBlock(extent_id, offset, router);
        if (!status.ok()) {
            return status;
        }
    }
    return reclaimExtent(extent_id, router);
}

Status GcManager::deleteOneBlock(
        const std::string &extent_id, uint64_t offset,
        const common::pb::ExtentRouter &router) {
    int replicas = 1 + router.secondaries().size();
    std::vector<common::ConnectionPtr> conns;
    if (getConns(router, &conns) != 0) {
        return Status(
                common::CYPRE_EM_GET_CONN_ERROR, "couldn't get connections");
    }
    std::vector<DeleteContext> ctxs(replicas);
    for (int i = 0; i < replicas; ++i) {
        extentserver::pb::ExtentIOService_Stub stub(conns[i]->channel.get());
        extentserver::pb::DeleteRequest req;
        req.set_extent_id(extent_id);
        req.set_offset(offset);
        req.set_size(block_size_);
        stub.Delete(&ctxs[i].cntl, &req, &ctxs[i].response, brpc::DoNothing());
    }

    for (int i = 0; i < replicas; ++i) {
        brpc::Join(ctxs[i].cntl.call_id());
        if (ctxs[i].cntl.Failed()) {
            LOG(ERROR) << "Couldn't send delete request: "
                       << ctxs[i].cntl.ErrorText();
            return Status(common::CYPRE_EM_BRPC_SEND_FAIL);
        } else if (ctxs[i].response.status().code() != 0) {
            LOG(ERROR) << "Couldn't delete: "
                       << ctxs[i].response.status().message();
            return Status(ctxs[i].response.status().code());
        }
    }
    return Status();
}

Status GcManager::reclaimExtent(
        const std::string &extent_id, const common::pb::ExtentRouter &router) {
    int replicas = 1 + router.secondaries().size();
    std::vector<common::ConnectionPtr> conns;
    if (getConns(router, &conns) != 0) {
        return Status(
                common::CYPRE_EM_GET_CONN_ERROR, "couldn't get connections");
    }
    std::vector<ReclaimContext> ctxs(replicas);
    for (int i = 0; i < replicas; ++i) {
        extentserver::pb::ExtentControlService_Stub stub(
                conns[i]->channel.get());
        extentserver::pb::ReclaimExtentRequest req;
        req.set_extent_id(extent_id);
        stub.ReclaimExtent(
                &ctxs[i].cntl, &req, &ctxs[i].response, brpc::DoNothing());
    }

    for (int i = 0; i < replicas; ++i) {
        brpc::Join(ctxs[i].cntl.call_id());
        if (ctxs[i].cntl.Failed()) {
            LOG(ERROR) << "Couldn't send reclaim extent request: "
                       << ctxs[i].cntl.ErrorText();
            return Status(common::CYPRE_EM_BRPC_SEND_FAIL);
        } else if (ctxs[i].response.status().code() != 0) {
            LOG(ERROR) << "Couldn't reclaim extent: "
                       << ctxs[i].response.status().message();
            return Status(ctxs[i].response.status().code());
        }
    }
    return Status();
}

Status GcManager::getRouter(
        const std::string &extent_id, common::pb::ExtentRouter *router) {
    std::string pool_id;
    std::string rg_id;
    auto status =
            ExtentManager::GlobalInstance().get_router_mgr()->query_router(
                    extent_id, &pool_id, &rg_id);
    if (!status.ok()) {
        return status;
    }
    auto es_group =
            ExtentManager::GlobalInstance().get_pool_mgr()->query_es_group(
                    pool_id, rg_id);
    ExtentManager::GlobalInstance().get_pool_mgr()->query_es_router(
            pool_id, es_group, router);
    return Status();
}

int GcManager::getConns(
        const common::pb::ExtentRouter &router,
        std::vector<common::ConnectionPtr> *conns) {
    auto conn = conn_pool_->GetConnection(
            router.primary().public_ip(), router.primary().public_port());
    if (conn == nullptr) {
        LOG(ERROR) << "Couldn't connect to peer es";
        return -1;
    }
    conns->push_back(conn);
    for (int i = 0; i < router.secondaries().size(); ++i) {
        auto conn = conn_pool_->GetConnection(
                router.secondaries(i).public_ip(),
                router.secondaries(i).public_port());
        if (conn == nullptr) {
            LOG(ERROR) << "Couldn't connect to peer es";
            return -1;
        }
        conns->push_back(conn);
    }
    return 0;
}

bool GcManager::timeValid() {
    time_t tt = time(NULL);
    tm *t = localtime(&tt);
    return t->tm_hour > GlobalConfig().extentmanager().gc_begin
           && t->tm_hour < GlobalConfig().extentmanager().gc_end;
}

}  // namespace extentmanager
}  // namespace cyprestore
