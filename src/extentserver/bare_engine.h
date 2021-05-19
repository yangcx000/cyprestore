/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_BARE_ENGINE_H
#define CYPRESTORE_EXTENTSERVER_BARE_ENGINE_H

#include <butil/macros.h>

#include <memory>

#include "block_device.h"
#include "common/status.h"
#include "extent_location.h"
#include "request_context.h"

namespace cyprestore {
namespace extentserver {

class StorageEngine;
class BareEngine;

typedef std::shared_ptr<BareEngine> BareEnginePtr;

using common::Status;

class BareEngine {
public:
    BareEngine(StorageEngine *se) : se_(se) {}
    ~BareEngine() {}

    Status Init();
    Status Close();
    Status DoRecovery();

    uint64_t Capacity() const {
        return bdev_->capacity();
    }

    const BlockDevicePtr &bdev() const {
        return bdev_;
    }
    const ExtentLocationMgrPtr &ExtentLocationMgr() const {
        return extent_loc_mgr_;
    }

    Status PeriodDeviceAdmin();
    Status ProcessRequest(Request *req);
    void SetExtentSize(uint64_t extent_size) {
        extent_loc_mgr_->SetExtentSize(extent_size);
    }
    uint64_t UsedSize() {
        return extent_loc_mgr_->UsedSize();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(BareEngine);
    Status bindBDev();
    Status unbindBDev();
    Status handleRead(Request *req);
    Status handleWrite(Request *req);
    Status handleReplicate(Request *req);
    Status handleDelete(Request *req);
    Status handleScrub(Request *req);
    Status handleReclaimExtent(Request *req);

    BlockDevicePtr bdev_;
    ExtentLocationMgrPtr extent_loc_mgr_;
    StorageEngine *se_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_BARE_ENGINE_H
