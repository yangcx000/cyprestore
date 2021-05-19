/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_EXTENTSERVER_H_
#define CYPRESTORE_EXTENTSERVER_EXTENTSERVER_H_

#include <brpc/channel.h>
#include <brpc/server.h>
#include <butil/macros.h>

#include <string>

#include "common/config.h"
#include "heartbeat_reporter.h"
#include "request_context.h"
#include "storage_engine.h"

namespace cyprestore {
namespace extentserver {

class ExtentServer {
public:
    enum ExtentServerStatus {
        kExtentServerOk = 0,
        kExtentServerStarting,
        kExtentServerReady,
        kExtentServerFailed,
        kExtentServerRecovering,
        kExtentServerRebalancing,
        kExtentServerUnknown = -1,
    };

    ExtentServer();
    ~ExtentServer() = default;

    static ExtentServer &GlobalInstance();
    static void SignHandler(int sig);

    int Init(const std::string &config_path);
    int RegisterServices();
    void Start();
    void Destroy();
    void WaitToQuit();

    int InstId() const {
        return instance_id_;
    }
    int GetSetID() const {
        return set_id_;
    }
    const std::string &InstName() const {
        return instance_name_;
    }
    const std::string &GetSetName() const {
        return set_name_;
    }
    const std::string &PoolId() const {
        return pool_id_;
    }

    brpc::Channel *GetEmChannel() const {
        return em_channel_;
    }
    const StorageEnginePtr &StorageEngine() const {
        return storage_engine_;
    }
    const RequestMgrPtr RequestMgr() const {
        return request_mgr_;
    }

    void SetEsReady() {
        status_ = kExtentServerReady;
    }
    void SetEsOk() {
        status_ = kExtentServerOk;
    }
    void WaitEsReady();

private:
    DISALLOW_COPY_AND_ASSIGN(ExtentServer);
    friend class HeartbeatReporter;
    void initMyself();

    // instance id && name
    int instance_id_;
    std::string instance_name_;

    // cluster id && name
    int set_id_;
    std::string set_name_;

    // pool && failure domain
    std::string pool_id_;
    std::string host_;
    std::string rack_;

    ExtentServerStatus status_;
    HeartbeatReporterPtr heartbeat_reporter_;
    StorageEnginePtr storage_engine_;
    RequestMgrPtr request_mgr_;
    brpc::Channel *em_channel_;
    brpc::Server server_;
    volatile bool stop_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_EXTENTSERVER_H_
