/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_STORAGE_ENGINE_H
#define CYPRESTORE_EXTENTSERVER_STORAGE_ENGINE_H

#include <butil/macros.h>

#include <memory>

#include "bare_engine.h"
#include "common/extent_router.h"
#include "common/status.h"
#include "replicate_engine.h"
#include "request_context.h"
//#include "log_engine.h"

namespace cyprestore {
namespace extentserver {

const std::string kHddDevice = "hdd";
const std::string kSsdDevice = "ssd";
const std::string kNVMeDevice = "nvme";

const std::string kStandardReplicationType = "standard";
const std::string kStarReplicationType = "star";

class StorageEngine;
typedef std::shared_ptr<StorageEngine> StorageEnginePtr;

using common::Status;

class StorageEngine {
public:
    enum EngineType {
        kHddEngine = 0,
        kSsdEngine,
        kNVMeEngine,
        kInvalidEngine = -1,
    };

    enum ReplicationType {
        kStandardReplication = 0,
        kStarReplication,
        kInvalidReplication = -1,
    };

    StorageEngine(
            const std::string &device_type,
            const std::string &replication_type);
    ~StorageEngine() = default;

    Status Init();
    Status Close();

    void SetExtentSize(uint64_t extent_size) {
        extent_size_ = extent_size;
        bare_engine_->SetExtentSize(extent_size);
    }
    uint64_t ExtentSize() const {
        return extent_size_;
    }
    uint64_t Capacity() const {
        return bare_engine_->Capacity();
    }
    uint64_t UsedSize() const {
        return bare_engine_->UsedSize();
    }

    const common::ExtentRouterMgrPtr &ExtentRouterMgr() const {
        return extent_router_mgr_;
    }

    Status PeriodDeviceAdmin();
    Status ProcessRequest(Request *req);

private:
    DISALLOW_COPY_AND_ASSIGN(StorageEngine);
    friend class BareEngine;
    Status doRecovery();
    Status doSafetyCheck(Request *req);
    Status doReplicate(Request *req);
    Status checkParameters(Request *req);
    Status checkOwnership(Request *req);
    Status checkChecksum(Request *req);
    Status queryRouter(Request *req);

    EngineType engine_type_;
    ReplicationType replication_type_;
    const uint32_t align_size_;  // 4K对齐
    uint64_t extent_size_;
    common::ExtentRouterMgrPtr extent_router_mgr_;
    bool enable_log_engine_;
    // read/write disk local
    BareEnginePtr bare_engine_;
    // replicate remote
    ReplicateEnginePtr replica_engine_;
    // read/write log local
    // LogEnginePtr log_engine_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_STORAGE_ENGINE_H
