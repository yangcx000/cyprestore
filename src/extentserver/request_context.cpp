/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "request_context.h"

#include "pb/extent_control.pb.h"
#include "pb/extent_io.pb.h"

namespace cyprestore {
namespace extentserver {

std::string Request::ExtentID() const {
    switch (request_type_) {
        case RequestType::kTypeRead:
            return (static_cast<pb::ReadRequest *>(op_ctx_.request))
                    ->extent_id();
        case RequestType::kTypeWrite:
            return (static_cast<pb::WriteRequest *>(op_ctx_.request))
                    ->extent_id();
        case RequestType::kTypeReplicate:
            return (static_cast<pb::ReplicateRequest *>(op_ctx_.request))
                    ->extent_id();
        case RequestType::kTypeScrub:
            return (static_cast<pb::ScrubRequest *>(op_ctx_.request))
                    ->extent_id();
        case RequestType::kTypeDelete:
            return (static_cast<pb::DeleteRequest *>(op_ctx_.request))
                    ->extent_id();
        case RequestType::kTypeReclaimExtent:
            return (static_cast<pb::ReclaimExtentRequest *>(op_ctx_.request))
                    ->extent_id();
        default:
            break;
    }

    return std::string();
}

uint64_t Request::Offset() const {
    switch (request_type_) {
        case RequestType::kTypeRead:
            return (static_cast<pb::ReadRequest *>(op_ctx_.request))->offset();
        case RequestType::kTypeWrite:
            return (static_cast<pb::WriteRequest *>(op_ctx_.request))->offset();
        case RequestType::kTypeReplicate:
            return (static_cast<pb::ReplicateRequest *>(op_ctx_.request))
                    ->offset();
        case RequestType::kTypeScrub:
            return (static_cast<pb::ScrubRequest *>(op_ctx_.request))->offset();
        case RequestType::kTypeDelete:
            return (static_cast<pb::DeleteRequest *>(op_ctx_.request))
                    ->offset();
        case RequestType::kTypeReclaimExtent:
            return 0;
        default:
            break;
    }

    return 0;
}

uint64_t Request::Size() const {
    switch (request_type_) {
        case RequestType::kTypeRead:
            return (static_cast<pb::ReadRequest *>(op_ctx_.request))->size();
        case RequestType::kTypeWrite:
            return (static_cast<pb::WriteRequest *>(op_ctx_.request))->size();
        case RequestType::kTypeReplicate:
            return (static_cast<pb::ReplicateRequest *>(op_ctx_.request))
                    ->size();
        case RequestType::kTypeScrub:
            return (static_cast<pb::ScrubRequest *>(op_ctx_.request))->size();
        case RequestType::kTypeDelete:
            return (static_cast<pb::DeleteRequest *>(op_ctx_.request))->size();
        case RequestType::kTypeReclaimExtent:
            return 0;
        default:
            break;
    }

    return 0;
}

void Request::SetOperationContext(
        brpc::Controller *cntl, google::protobuf::Message *request,
        google::protobuf::Message *response, google::protobuf::Closure *done) {
    op_ctx_.cntl = cntl;
    op_ctx_.request = request;
    op_ctx_.response = response;
    op_ctx_.done = done;
}

int RequestMgr::Init() {
    auto ctxmem_mgr = new common::CtxMemMgr<Request>(
            "request", CypreRing::CypreRingType::TYPE_MP_MC, true);
    if (ctxmem_mgr == nullptr) {
        LOG(ERROR) << "Create io request manager failed.";
        return -1;
    }
    ctxmem_mgr_.reset(ctxmem_mgr);
    auto s = ctxmem_mgr_->Init();
    if (!s.ok()) {
        LOG(ERROR) << "Io request manager init failed.";
        return -1;
    }
    return 0;
}

Request *RequestMgr::GetRequest(RequestType request_type) {
    Request *req = nullptr;
    auto status = ctxmem_mgr_->GetCtxUnitBulk(&req);
    if (!status.ok()) {
        LOG(ERROR) << "get io request unit failed.";
        return nullptr;
    }
    req->Reset(request_type);
    return req;
}

void RequestMgr::PutRequest(Request *req) {
    req->Reset(RequestType::kTypeNoop);
    ctxmem_mgr_->PutCtxUnit(req);
}

}  // namespace extentserver
}  // namespace cyprestore
