/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "replicate_engine.h"

#include <butil/logging.h>

#include "pb/extent_io.pb.h"

namespace cyprestore {
namespace extentserver {

Status ReplicateEngine::Send(Request *req) {
    // 获取路由
    auto extent_router = req->GetExtentRouter();
    bool failed = false;
    req->SetRefCount(1 + extent_router->secondaries.size());
    for (size_t i = 0; i < extent_router->secondaries.size(); ++i) {
        if (failed) {
            req->UserCallback()(req);
            continue;
        }
        auto conn = conn_pool_->GetConnection(
                extent_router->secondaries[i].public_ip,
                extent_router->secondaries[i].public_port);
        if (!conn) {
            LOG(ERROR) << "Couldn't connect to peer es"
                       << ", extent_id:" << req->ExtentID()
                       << ", offset:" << req->Offset()
                       << ", size:" << req->Size() << ", peer_addr:"
                       << extent_router->secondaries[i].address();
            failed = true;
            req->SetResult(false);
            req->UserCallback()(req);
            continue;
        }
        sendOneReplicate(req, conn);
    }

    if (failed) {
        return Status(
                common::CYPRE_ES_REPLICATE_PART_FAIL,
                "replicate partial error");
    }
    return Status();
}

void ReplicateEngine::sendOneReplicate(
        Request *req, common::ConnectionPtr conn) {
    // TODO(yangchunxin3): 避免内存分配
    brpc::Controller *cntl = new brpc::Controller();
    pb::ReplicateResponse *repl_resp = new pb::ReplicateResponse();
    pb::ExtentIOService_Stub stub(conn->channel.get());
    pb::ReplicateRequest repl_req;
    repl_req.set_extent_id(req->ExtentID());
    repl_req.set_offset(req->Offset());
    repl_req.set_size(req->Size());

    auto &op_ctx = req->GetOperationContext();
    pb::WriteRequest *request = static_cast<pb::WriteRequest *>(op_ctx.request);
    if (request->has_crc32()) {
        repl_req.set_crc32(request->crc32());
    }
    cntl->request_attachment() = req->GetOperationContext().cntl->request_attachment();
    google::protobuf::Closure *done = brpc::NewCallback<brpc::Controller*,
            pb::ReplicateResponse*,
            void*>
            (&ReplicateEngine::HandleResponse, cntl, repl_resp, req);
    stub.Replicate(cntl, &repl_req, repl_resp, done);
}

void ReplicateEngine::HandleResponse(
        brpc::Controller *cntl, pb::ReplicateResponse *repl_resp, void *arg) {
    std::unique_ptr<brpc::Controller> cntl_guard(cntl);
    std::unique_ptr<pb::ReplicateResponse> response_guard(repl_resp);
    bool success = false;
    if (cntl->Failed()) {
        LOG(ERROR) << "Couldn't send replicate request, " << cntl->ErrorText();
    } else if (repl_resp->status().code() != 0) {
        LOG(ERROR) << "Couldn't replicate extent, "
                   << repl_resp->status().message();
    } else {
        success = true;
    }

    Request *req = static_cast<Request *>(arg);
    req->SetResult(success);
    req->UserCallback()(req);
}

}  // namespace extentserver
}  // namespace cyprestore
