/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "storage_engine.h"

#include "common/config.h"
#include "common/constants.h"
#include "extentserver.h"
#include "utils/crc32.h"

namespace cyprestore {
namespace extentserver {

StorageEngine::StorageEngine(
        const std::string &device_type, const std::string &replication_type)
        : engine_type_(StorageEngine::kInvalidEngine),
          replication_type_(StorageEngine::kInvalidReplication),
          align_size_(common::kAlignSize), extent_size_(0),
          enable_log_engine_(false) {
    if (device_type == kHddDevice) {
        engine_type_ = kHddEngine;
    } else if (device_type == kSsdDevice) {
        engine_type_ = kSsdEngine;
    } else if (device_type == kNVMeDevice) {
        engine_type_ = kNVMeEngine;
    }

    if (replication_type == kStandardReplicationType) {
        replication_type_ = kStandardReplication;
    } else if (replication_type == kStarReplicationType) {
        replication_type_ = kStarReplication;
    }
}

Status StorageEngine::Init() {
    if (replication_type_ < 0) {
        return Status(
                common::CYPRE_ER_NOT_SUPPORTED,
                "replication type not supported");
    }

    bare_engine_.reset(new BareEngine(this));
    Status s = bare_engine_->Init();
    if (!s.ok()) return s;

    extent_router_mgr_.reset(new common::ExtentRouterMgr(
            ExtentServer::GlobalInstance().GetEmChannel(), false));
    replica_engine_.reset(new ReplicateEngine(extent_router_mgr_));
    return doRecovery();
}

Status StorageEngine::Close() {
    return bare_engine_->Close();
}

Status StorageEngine::doRecovery() {
    return bare_engine_->DoRecovery();
}

Status StorageEngine::queryRouter(Request *req) {
    if (req->GetRequestType() == RequestType::kTypeDelete
        || req->GetRequestType() == RequestType::kTypeReclaimExtent) {
        return Status();
    }

    auto extent_router = extent_router_mgr_->QueryRouter(req->ExtentID());
    if (!extent_router) {
        return Status(
                common::CYPRE_ES_QUERY_ROUTER_FAIL,
                "couldn't get extent router");
    }
    req->SetExtentRouter(extent_router);
    return Status();
}

Status StorageEngine::PeriodDeviceAdmin() {
    return bare_engine_->PeriodDeviceAdmin();
}

Status StorageEngine::ProcessRequest(Request *req) {
    auto status = doSafetyCheck(req);
    if (!status.ok()) {
        return status;
    }

    status = doReplicate(req);
    if (!status.ok()) {
        return status;
    }

    if (!enable_log_engine_) {
        return bare_engine_->ProcessRequest(req);
    }

    // calls log_engine_
    return Status();
}

Status StorageEngine::doReplicate(Request *req) {
    if (replication_type_ == kStarReplication) {
        return Status();
    }
    if (req->GetRequestType() != RequestType::kTypeWrite) {
        return Status();
    }
    return replica_engine_->Send(req);
}

Status StorageEngine::checkParameters(Request *req) {
    if (req->ExtentID().empty()) {
        return Status(common::CYPRE_ER_INVALID_ARGUMENT, "extent_id empty");
    }

    if (req->Offset() % align_size_ != 0) {
        return Status(
                common::CYPRE_ER_INVALID_ARGUMENT, "offset not align with 4k");
    }

    if (req->Size() % align_size_ != 0) {
        return Status(
                common::CYPRE_ER_INVALID_ARGUMENT, "len not align with 4k");
    }

    if (req->Offset() + req->Size() > extent_size_) {
        return Status(
                common::CYPRE_ER_INVALID_ARGUMENT,
                "(offset + len) exceeds extent size");
    }

    return Status();
}

Status StorageEngine::checkOwnership(Request *req) {
    auto extent_router = req->GetExtentRouter();
    auto ip = GlobalConfig().network().public_ip;
    auto port = GlobalConfig().network().public_port;
    switch (req->GetRequestType()) {
        case RequestType::kTypeRead: {
            if (!extent_router->IsPrimary(ip, port)) {
                return Status(
                        common::CYPRE_ER_NO_PERMISSION,
                        "ilegal node, not primary");
            }
        }
        case RequestType::kTypeWrite: {
            if (replication_type_ == kStandardReplication
                && !extent_router->IsPrimary(ip, port)) {
                return Status(
                        common::CYPRE_ER_NO_PERMISSION,
                        "ilegal node, not primary");
            } else if (
                    replication_type_ == kStarReplication
                    && !extent_router->IsValid(ip, port)) {
                return Status(
                        common::CYPRE_ER_NO_PERMISSION,
                        "ilegal node, not primary or secondary");
            }
            break;
        }
        case RequestType::kTypeReplicate: {
            if (!extent_router->IsSecondary(ip, port)) {
                return Status(
                        common::CYPRE_ER_NO_PERMISSION,
                        "ilegal node, not secondary");
            }
            break;
        }
        case RequestType::kTypeScrub: {
            if (!extent_router->IsValid(ip, port)) {
                return Status(
                        common::CYPRE_ER_NO_PERMISSION,
                        "illegal node, not primary or secondary");
            }
            break;
        }
        // no need check
        case RequestType::kTypeReclaimExtent:
        case RequestType::kTypeDelete: {
            break;
        }
        default:
            return Status(
                    common::CYPRE_ER_INVALID_ARGUMENT, "invalid cmd type");
    }
    return Status();
}

Status StorageEngine::checkChecksum(Request *req) {
    switch(req->GetRequestType()) {
        case RequestType::kTypeWrite: {
            auto &op_ctx = req->GetOperationContext();
            pb::WriteRequest *request = static_cast<pb::WriteRequest *>(op_ctx.request);
            if (request->has_crc32()) {
                auto actual_crc32 = utils::Crc32::Checksum(op_ctx.cntl->request_attachment());
                if (actual_crc32 == request->crc32()) { return Status(); }
                LOG(ERROR) << "crc32 value not equal"
                           << ", actual crc32: " << actual_crc32
                           << ", expected crc32: " << request->crc32()
                           << ", extent id: " << request->extent_id()
                           << ", offset: " << request->offset()
                           << ", size: " << request->size();
                return Status(common::CYPRE_ES_CHECKSUM_ERROR, "crc32 value not equal to data checksum");
            }
            break;
        }
        case RequestType::kTypeReplicate: {
            auto &op_ctx = req->GetOperationContext();
            pb::ReplicateRequest *request = static_cast<pb::ReplicateRequest *>(op_ctx.request);
            if (request->has_crc32()) {
                auto actual_crc32 = utils::Crc32::Checksum(op_ctx.cntl->request_attachment());
                if (actual_crc32 == request->crc32()) { return Status(); }
                LOG(ERROR) << "crc32 value not equal"
                           << ", actual crc32: " << actual_crc32
                           << ", expected crc32: " << request->crc32()
                           << ", extent id: " << request->extent_id()
                           << ", offset: " << request->offset()
                           << ", size: " << request->size();
                return Status(common::CYPRE_ES_CHECKSUM_ERROR, "crc32 value not equal to data checksum");
            }
            break;
        }
        default:
            return Status();
    }
    return Status();
}

Status StorageEngine::doSafetyCheck(Request *req) {
    auto s = queryRouter(req);
    if (!s.ok()) return s;

    s = checkParameters(req);
    if (!s.ok()) return s;

    s = checkOwnership(req);
    if (!s.ok()) return s;

    return checkChecksum(req);
}

}  // namespace extentserver
}  // namespace cyprestore
