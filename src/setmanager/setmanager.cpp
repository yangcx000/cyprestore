/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "setmanager.h"

#include <butil/logging.h>
#include <unistd.h>

#include <csignal>

#include "common/log.h"
#include "setmanager/heartbeat_service.h"
#include "setmanager/set_service.h"

namespace cyprestore {
namespace setmanager {

SetManager &SetManager::GlobalInstance() {
    static SetManager globalInstance;
    return globalInstance;
}

int SetManager::Init(const std::string &config_path) {
    // 1. 配置初始化 XXX: 替换字符串常量
    if (GlobalConfig().Init("setmanager", config_path) != 0) {
        return -1;
    }

    // 2. 日志初始化
    common::Log log(GlobalConfig().log());
    if (log.Init() != 0) {
        return -1;
    }

    return 0;
}

int SetManager::RegisterServices() {
    SetServiceImpl *set_service = new SetServiceImpl();
    if (server_.AddService(set_service, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Add [SetService]failed";
        return -1;
    }

    HeartbeatServiceImpl *heartbeat_service = new HeartbeatServiceImpl();
    if (server_.AddService(heartbeat_service, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Add [HeartbeatService] failed";
        return -1;
    }

    return 0;
}

void SignHandler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            SetManager::GlobalInstance().Destroy();
            break;
    }
}

void SetManager::Start() {
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
    LOG(INFO) << "SetManager server has started";
    stop_ = false;
    WaitToQuit();
}

void SetManager::WaitToQuit() {
    while (!stop_) {
        usleep(100);
    }
    LOG(INFO) << "SetManager server has stoped";
}

void SetManager::Destroy() {
    server_.Stop(0);
    server_.Join();
    stop_ = true;
}

Set SetManager::AllocateSet(common::pb::PoolType pool_type) {
    int set_id = -1;
    uint64_t num_extents = 0;
    double minimun_capacity_utils = 1.0;

    std::lock_guard<std::mutex> lock(mutex_);
    // 遍历sets
    for (auto it = set_map_.begin();
         it != set_map_.end() && !it->second->obsoleted(); ++it) {
        // 遍历set内的pools
        for (int k = 0; k < it->second->pb_set.pools_size(); ++k) {
            auto &pool = it->second->pb_set.pools(k);
            // pool类型不匹配或者pool空间为0
            if (pool.type() != pool_type || pool.capacity() == 0
                || pool.status() != common::pb::POOL_STATUS_ENABLED) {
                LOG(DEBUG) << "Skip pool " << pool.id()
                           << ", pool_type:" << pool.type()
                           << ", pool_capacity:" << pool.capacity()
                           << ", pool_status:" << pool.status();
                continue;
            }

            double capacity_utils = pool.size() / (double)pool.capacity();
            if (capacity_utils < minimun_capacity_utils) {
                minimun_capacity_utils = capacity_utils;
                set_id = it->first;
                num_extents = pool.num_extents();
            } else if (
                    capacity_utils == minimun_capacity_utils
                    && pool.num_extents() < num_extents) {
                set_id = it->first;
                num_extents = pool.num_extents();
            }
        }
    }

    if (set_id == -1) {
        return Set();
    }

    return *set_map_[set_id];
}

void SetManager::AddSet(const common::pb::Set &pb_set) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = set_map_.find(pb_set.id());
    if (it == set_map_.end()) {
        set_map_.insert(
                std::make_pair(pb_set.id(), std::make_shared<Set>(pb_set)));
        return;
    }

    it->second->pb_set = pb_set;
    it->second->created_time = utils::Chrono::CurrentTime().tv_sec;
}

SetArray SetManager::ListSets() {
    std::lock_guard<std::mutex> lock(mutex_);
    SetArray copy_set;
    for (auto it = set_map_.begin();
         it != set_map_.end() && !it->second->obsoleted(); ++it) {
        copy_set.push_back(*(it->second));
    }
    return copy_set;
}

}  // namespace setmanager
}  // namespace cyprestore
