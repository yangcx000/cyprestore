/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "extent_manager.h"

#include <brpc/channel.h>
#include <butil/logging.h>
#include <unistd.h>

#include <cereal/archives/binary.hpp>
#include <csignal>
#include <sstream>
#include <vector>

#include "cluster_service.h"
#include "common/constants.h"
#include "common/log.h"
#include "common/pb/types.pb.h"
#include "common/status.h"
#include "heartbeat_service.h"
#include "resource_service.h"
#include "router_service.h"
#include "setmanager/pb/heartbeat.pb.h"
#include "utils/chrono.h"
#include "utils/pb_transfer.h"
#include "utils/timer_thread.h"
#include "utils/uuid.h"

namespace cyprestore {
namespace extentmanager {

ExtentManager &ExtentManager::GlobalInstance() {
    static ExtentManager globalInstance;
    return globalInstance;
}

ExtentManager::ExtentManager() {
    inst_id_ = GlobalConfig().common().instance;
    inst_name_ = "em-" + std::to_string(inst_id_);

    set_id_ = GlobalConfig().common().set_id;
    set_name_ =
            GlobalConfig().common().set_prefix + "-" + std::to_string(set_id_);
}

ExtentManager::~ExtentManager() {}

void ExtentManager::HeartbeatFunc(void *arg) {
    ExtentManager *em = reinterpret_cast<ExtentManager *>(arg);
    em->ReportHeartbeat();
}

void ExtentManager::ReportHeartbeat() {
    brpc::Controller cntl;
    setmanager::pb::HeartbeatRequest request;
    setmanager::pb::HeartbeatResponse response;
    setmanager::pb::HeartbeatService_Stub hb_stub(sm_channel_.get());

    common::pb::Set set;
    set.set_id(set_id_);
    set.set_name(set_name_);

    common::pb::ExtentManager em;
    em.set_id(inst_id_);
    em.set_name(inst_name_);

    common::pb::Endpoint endpoint;
    endpoint.set_public_ip(GlobalConfig().network().public_ip);
    endpoint.set_public_port(GlobalConfig().network().public_port);

    *em.mutable_endpoint() = endpoint;
    *set.mutable_em() = em;

    std::vector<common::pb::Pool> pools;
    auto status =
            ExtentManager::GlobalInstance().get_pool_mgr()->list_pools(&pools);
    for (uint32_t i = 0; i < pools.size(); i++) {
        *set.add_pools() = pools[i];
    }

    *request.mutable_set() = set;
    hb_stub.ReportHeartbeat(&cntl, &request, &response, NULL);
    if (cntl.Failed() || response.status().code() != 0) {
        // XXX: print setmanager address, address从etcd获取
        LOG(ERROR) << "Couldn't report heartbeat to setmanager";
    }
}

uint64_t ExtentManager::get_extent_size() {
    return GlobalConfig().extentmanager().extent_size;
}

int ExtentManager::Init(const std::string &config_path) {
    // 1. init config
    if (GlobalConfig().Init("extentmanager", config_path) != 0) {
        return -1;
    }

    // 2. init log
    common::Log log(GlobalConfig().log());
    if (log.Init() != 0) {
        return -1;
    }

    // 3. init SetManager channel
    sm_channel_.reset(new brpc::Channel());
    // XXX: get address from etcd
    std::string set_ip = GlobalConfig().setmanager().set_ip;
    int set_port = GlobalConfig().setmanager().set_port;
    if (set_ip.empty() || set_port == 0) {
        LOG(ERROR) << "setmanager ip or port is not configed.";
        return -1;
    }
    std::stringstream ss(std::ios_base::app | std::ios_base::out);
    ss << set_ip << ":" << set_port;
    if (sm_channel_->Init(ss.str().c_str(), NULL) != 0) {
        LOG(ERROR) << "Couldn't connect setmanager: " << ss.str();
        return -1;
    }

    // 4. init KV store
    kvstore::RocksOption rocks_option;
    rocks_option.db_path = GlobalConfig().rocksdb().db_path;
    rocks_option.compact_threads = GlobalConfig().rocksdb().compact_threads;
    kv_store_.reset(new kvstore::RocksStore(rocks_option));
    auto status = kv_store_->Open();
    if (!status.ok()) {
        LOG(ERROR) << "Open rocksdb failed, status: " << status.ToString();
        return -1;
    }

    // 5. init PoolManager
    pool_mgr_.reset(new PoolManager(
            kv_store_, GlobalConfig().extentmanager().extent_size));

    // 6. init UserManager
    user_mgr_.reset(new UserManager(kv_store_));

    // 7. init RouterManager
    router_mgr_.reset(new RouterManager(kv_store_));

    // 8. init GcManager
    gc_mgr_.reset(new GcManager());
    if (gc_mgr_->Init() != 0) {
        LOG(ERROR) << "create gc timer faild.";
        return -1;
    }

    // 9. recovery from KV store
    if (DoRecovery() != 0) {
        LOG(ERROR) << "Recovery meta info from stroe faild.";
        return -1;
    }

    // 10. init timer thread
    utils::TimerThread *tt = utils::GetOrCreateGlobalTimerThread();
    if (!tt) {
        LOG(ERROR) << "Couldn't create timer thread";
        return -1;
    }

    utils::TimerOptions to(
            true, GlobalConfig().extentmanager().heartbeat_interval_sec * 1000,
            ExtentManager::HeartbeatFunc, this);
    if (tt->create_timer(to) != 0) {
        LOG(ERROR) << "Couldn't create heartbeat timer";
        return -1;
    }

    return 0;
}

int ExtentManager::RegisterServices() {
    ClusterServiceImpl *cluster_service = new ClusterServiceImpl();
    if (server_.AddService(cluster_service, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Add [ClusterService]failed";
        return -1;
    }

    HeartbeatServiceImpl *heartbeat_service = new HeartbeatServiceImpl();
    if (server_.AddService(heartbeat_service, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Add [HeartbeatService] failed";
        return -1;
    }

    RouterServiceImpl *router_service = new RouterServiceImpl();
    if (server_.AddService(router_service, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Add [RouterService] failed";
        return -1;
    }

    ResourceServiceImpl *resource_service = new ResourceServiceImpl();
    if (server_.AddService(resource_service, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Add [ResourceService] failed";
        return -1;
    }
    return 0;
}

void SignHandler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            ExtentManager::GlobalInstance().Destroy();
            break;
    }
}

void ExtentManager::Start() {
    brpc::ServerOptions options;
    options.num_threads = GlobalConfig().brpc().num_threads;
    options.max_concurrency = GlobalConfig().brpc().max_concurrency;
    std::string endpoint = GlobalConfig().network().public_endpoint();

    if (server_.Start(endpoint.c_str(), &options) != 0) {
        LOG(ERROR) << "Brpc server failed to start on " << endpoint;
        return;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, SignHandler);
    signal(SIGTERM, SignHandler);
    LOG(INFO) << "ExtentManager server has started";
    stop_ = false;
    WaitToQuit();
}

void ExtentManager::WaitToQuit() {
    while (!stop_) {
        usleep(100);
    }
    LOG(INFO) << "ExtentManager server has stoped";
}

void ExtentManager::Destroy() {
    // param in stop means nothing
    server_.Stop(0);
    server_.Join();

    auto s = kv_store_->Close();
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't close kv_store, " << s.ToString();
        return;
    }
    stop_ = true;
}

int ExtentManager::DoRecovery() {
    // 1. recvoer pool meta info(include extentserver, replication group, and
    // blob meta info)
    auto success = pool_mgr_->recovery_from_store();
    if (!success) {
        LOG(ERROR) << "recovery pool meta info from store failed.";
        return -1;
    }

    // 2. recover user meta info
    success = user_mgr_->recovery_from_store();
    if (!success) {
        LOG(ERROR) << "recovery user meta info from store failed.";
        return -1;
    }
    // no need to recover router info because router info too many
    return 0;
}

}  // namespace extentmanager
}  // namespace cyprestore
