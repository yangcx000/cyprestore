/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "extentserver.h"

#include <butil/logging.h>
#include <unistd.h>

#include <csignal>

#include "common/log.h"
#include "extent_control_service.h"
#include "extent_io_service.h"
#include "utils/tools.h"

namespace cyprestore {
namespace extentserver {

ExtentServer &ExtentServer::GlobalInstance() {
    static ExtentServer globalInstance;
    return globalInstance;
}

ExtentServer::ExtentServer()
        : instance_id_(-1), set_id_(-1),
          status_(ExtentServer::kExtentServerStarting), em_channel_(nullptr),
          stop_(false) {}

void ExtentServer::initMyself() {
    instance_id_ = GlobalConfig().common().instance;
    instance_name_ = "es-" + std::to_string(instance_id_);

    set_id_ = GlobalConfig().common().set_id;
    set_name_ =
            GlobalConfig().common().set_prefix + "-" + std::to_string(set_id_);

    pool_id_ = GlobalConfig().extentserver().pool_id;
    rack_ = GlobalConfig().extentserver().rack;
    host_ = utils::GetHostName();
}

int ExtentServer::Init(const std::string &config_path) {
    if (GlobalConfig().Init("extentserver", config_path) != 0) {
        return -1;
    }

    int num_threads = GlobalConfig().brpc().num_threads;
    std::string core_mask = GlobalConfig().brpc().brpc_worker_core_mask;
    int ret = bthread_set_concurrency_and_cpu_affinity(num_threads, core_mask.c_str());
    if (ret != 0) {
        LOG(ERROR) << "Couldn't bind brpc worker to cpu cores, ret: " << ret;
        return -1;
    }

    initMyself();
    // init log
    common::Log log(GlobalConfig().log());
    if (log.Init() != 0) {
        return -1;
    }

    // create em client
    em_channel_ = new brpc::Channel();
    auto em_ip = GlobalConfig().extentmanager().em_ip;
    auto em_port = GlobalConfig().extentmanager().em_port;
    std::stringstream ss(em_ip, std::ios_base::app | std::ios_base::out);
    ss << ":" << em_port;
    std::string endpoint = ss.str();
    if (em_channel_->Init(endpoint.c_str(), NULL) != 0) {
        LOG(ERROR) << "Couldn't connect to extentmanager: " << endpoint;
        return -1;
    }

    storage_engine_.reset(new extentserver::StorageEngine(
            GlobalConfig().extentserver().dev_type,
            GlobalConfig().extentserver().replication_type));
    Status s = storage_engine_->Init();
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't init storage engine, " << s.ToString();
        return -1;
    }

    request_mgr_.reset(new extentserver::RequestMgr());
    if (request_mgr_->Init() != 0) {
        LOG(ERROR) << "Couldn't init io request manager";
        return -1;
    }

    heartbeat_reporter_.reset(new HeartbeatReporter(
            GlobalConfig().extentserver().heartbeat_interval_sec, this));
    s = heartbeat_reporter_->Start();
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't start heartbeat reporter, " << s.ToString();
        return -1;
    }

    LOG(INFO) << "Init extentserver finished";
    return 0;
}

int ExtentServer::RegisterServices() {
    ExtentIOServiceImpl *extent_io_service = new ExtentIOServiceImpl();
    if (server_.AddService(extent_io_service, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Couldn't add [ExtentIOService] service";
        return -1;
    }

    ExtentControlServiceImpl *extent_control_service =
            new ExtentControlServiceImpl();
    if (server_.AddService(extent_control_service, brpc::SERVER_OWNS_SERVICE)
        != 0) {
        LOG(ERROR) << "Couldn't add [ExtentControlService] service";
        return -1;
    }

    LOG(INFO) << "Register services finished";
    return 0;
}

void ExtentServer::SignHandler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            ExtentServer::GlobalInstance().Destroy();
            break;
    }
}

void ExtentServer::WaitEsReady() {
    while (status_ == kExtentServerStarting) {
        sleep(1);
    }
}

void ExtentServer::Start() {
    WaitEsReady();

    std::string endpoint = GlobalConfig().network().public_endpoint();
    if (server_.Start(endpoint.c_str(), nullptr) != 0) {
        LOG(ERROR) << "Couldn't start server on " << endpoint;
        return;
    }

    SetEsOk();

    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, ExtentServer::SignHandler);
    signal(SIGTERM, ExtentServer::SignHandler);

    LOG(INFO) << "ExtentServer has started";

    stop_ = false;
    WaitToQuit();
}

void ExtentServer::WaitToQuit() {
    Status s;
    while (!stop_) {
        usleep(100);
        s = storage_engine_->PeriodDeviceAdmin();
        if (!s.ok()) {
            LOG(WARNING) << "Couldn't do spdk poll, " << s.ToString();
        }
    }
    LOG(INFO) << "ExtentServer has stopped";
}

void ExtentServer::Destroy() {
    // param in stop means nothing
    server_.Stop(0);
    server_.Join();

    Status s = storage_engine_->Close();
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't close storage engine, " << s.ToString();
        return;
    }

    stop_ = true;
}

}  // namespace extentserver
}  // namespace cyprestore
