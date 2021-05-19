/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "bare_engine.h"

#include "common/config.h"
#include "nvme_device.h"
#include "storage_engine.h"

namespace cyprestore {
namespace extentserver {

Status BareEngine::bindBDev() {
    switch (se_->engine_type_) {
        case StorageEngine::kHddEngine:
        case StorageEngine::kSsdEngine:
            // XXX(yangchunxin3): not supported yet
            break;
        case StorageEngine::kNVMeEngine:
            bdev_.reset(new NVMeDevice(GlobalConfig().extentserver().dev_name));
            break;
        default:
            break;
    }

    if (!bdev_) {
        // TODO(yangchunxin3): return engine name
        return Status(
                common::CYPRE_ER_NOT_SUPPORTED,
                "storage engine not supported yet");
    }

    Status s = bdev_->InitEnv();
    if (!s.ok()) return s;

    return bdev_->Open();
}

Status BareEngine::Init() {
    Status s = bindBDev();
    if (!s.ok()) return s;

    extent_loc_mgr_.reset(new extentserver::ExtentLocationMgr());
    return extent_loc_mgr_->Init(bdev_->capacity());
}

Status BareEngine::unbindBDev() {
    if (!bdev_) {
        return Status(
                common::CYPRE_ES_BDEV_NULL,
                "storage engine hasn't been initialized yet");
    }

    Status s = bdev_->Close();
    if (!s.ok()) return s;

    return bdev_->CloseEnv();
}

Status BareEngine::Close() {
    Status s = extent_loc_mgr_->Close();
    if (!s.ok()) return s;

    return unbindBDev();
}

Status BareEngine::DoRecovery() {
    return extent_loc_mgr_->LoadExtents();
}

Status BareEngine::handleRead(Request *req) {
    auto status = extent_loc_mgr_->QueryLocation(req->ExtentID(), req, false);
    if (!status.ok()) {
        req->SetEmptyResponse();
        return Status(common::CYPRE_ES_EXTENT_EMPTY, "extent empty");
    }
    return bdev_->ProcessRequest(req);
}

Status BareEngine::handleWrite(Request *req) {
    auto status = extent_loc_mgr_->QueryLocation(req->ExtentID(), req);
    if (!status.ok()) {
        return status;
    }
    return bdev_->ProcessRequest(req);
}

Status BareEngine::handleDelete(Request *req) {
    auto status = extent_loc_mgr_->QueryLocation(req->ExtentID(), req, false);
    if (!status.ok()) {
        return status;
    }
    return bdev_->ProcessRequest(req);
}

Status BareEngine::handleScrub(Request *req) {
    auto status = extent_loc_mgr_->QueryLocation(req->ExtentID(), req, false);
    if (!status.ok()) {
        return status;
    }
    return bdev_->ProcessRequest(req);
}

Status BareEngine::handleReclaimExtent(Request *req) {
    se_->ExtentRouterMgr()->DeleteRouter(req->ExtentID());
    return extent_loc_mgr_->ReclaimExtent(req->ExtentID());
}

Status BareEngine::PeriodDeviceAdmin() {
    switch (se_->engine_type_) {
        case StorageEngine::kHddEngine:
        case StorageEngine::kSsdEngine:
            // XXX(yangchunxin3): not supported yet
            break;
        case StorageEngine::kNVMeEngine:
            return bdev_->PeriodDeviceAdmin();
        default:
            break;
    }

    return Status();
}

Status BareEngine::ProcessRequest(Request *req) {
    switch (req->GetRequestType()) {
        case RequestType::kTypeRead:
            return handleRead(req);
        case RequestType::kTypeWrite:
        case RequestType::kTypeReplicate:
            return handleWrite(req);
        case RequestType::kTypeDelete:
            return handleDelete(req);
        case RequestType::kTypeScrub:
            return handleScrub(req);
        case RequestType::kTypeReclaimExtent:
            return handleReclaimExtent(req);
        default:
            break;
    }

    return Status(common::CYPRE_ER_NOT_SUPPORTED, "invalid request type");
}

}  // namespace extentserver
}  // namespace cyprestore
