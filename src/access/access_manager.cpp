/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "access_manager.h"

#include <brpc/channel.h>
#include <butil/logging.h>
#include <unistd.h>

#include <csignal>

#include "access_service.h"
#include "common/log.h"
#include "extentmanager/pb/resource.pb.h"
#include "setmanager/pb/set.pb.h"

namespace cyprestore {
namespace access {

Access &Access::GlobalInstance() {
    static Access globalInstance;
    return globalInstance;
}

int Access::Init(const std::string &config_path) {
    // 1. init config
    if (GlobalConfig().Init("access", config_path) != 0) {
        return -1;
    }

    // 2. init log
    common::Log log(GlobalConfig().log());
    if (log.Init() != 0) {
        return -1;
    }

    // 3. init setManager channel
    if (InitSetManager() != 0) {
        return -1;
    }

    // 4. init extentmanager channel
    if (InitExtentManager() != 0) {
        return -1;
    }

    return 0;
}

int Access::InitSetManager() {
    set_mgr_.reset(new SetMgr());
    return set_mgr_->Init();
}

int Access::InitExtentManager() {
    resource_mgr_.reset(new ResourceMgr());
    return 0;
}

int Access::RegistreServices() {
    AccessServiceImpl *access_service = new AccessServiceImpl();
    if (server_.AddService(access_service, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Add access service failed";
        return -1;
    }
    return 0;
}

void SignHandler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            Access::GlobalInstance().Destroy();
            break;
    }
}

void Access::Start() {
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
    LOG(INFO) << "access has started";
    stop_ = false;
    WaitToQuit();
}

void Access::WaitToQuit() {
    while (!stop_) {
        usleep(100);
    }
    LOG(INFO) << "ExtentManager server has stoped";
}

void Access::Destroy() {
    // param in stop means nothing
    server_.Stop(0);
    ;
    server_.Join();
    stop_ = true;
}

}  // namespace access
}  // namespace cyprestore
