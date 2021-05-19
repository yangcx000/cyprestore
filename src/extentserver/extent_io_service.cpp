/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "extent_io_service.h"

#include <brpc/server.h>

#include <string>

#include "extentserver.h"
#include "request_context.h"
#include "storage_engine.h"
#include "utils/crc32.h"

namespace cyprestore {
namespace extentserver {

void ExtentIOServiceImpl::ReclaimIOUnit(void *arg) {
    Request *req = static_cast<Request *>(arg);
    req->GetIOMemMgr()->PutIOUnit(req->IOUnit());
    ExtentServer::GlobalInstance().RequestMgr()->PutRequest(req);
}

void* ExtentIOServiceImpl::ReadDone(void *arg) {
    Request *req = static_cast<Request *>(arg);
    auto &op_ctx = req->GetOperationContext();
    brpc::ClosureGuard done_guard(op_ctx.done);

    pb::ReadRequest *request = static_cast<pb::ReadRequest *>(op_ctx.request);
    pb::ReadResponse *response =
            static_cast<pb::ReadResponse *>(op_ctx.response);
    bool reclaimed = false;
    if (!req->Result()) {
        LOG(ERROR) << "Couldn't read extent from block device"
                   << ", extent_id: " << request->extent_id()
                   << ", offset: " << request->offset()
                   << ", size: " << request->size();
        response->mutable_status()->set_code(
                common::CYPRE_ES_PROCESS_REQ_ERROR);
        response->mutable_status()->set_message("read io error");
    } else {
        if (req->IOUnit() != nullptr) {
            req->EndTraceTime();
            op_ctx.cntl->response_attachment().append_user_data(
                    req->IOUnit()->data, req->Size(), ReclaimIOUnit, req);
            reclaimed = true;
        }
        response->mutable_status()->set_code(common::CYPRE_OK);
    }
    if (req->IOUnit() != nullptr && !reclaimed) {
        req->GetIOMemMgr()->PutIOUnit(req->IOUnit());
    }

    if (!reclaimed) {
        req->EndTraceTime();
        ExtentServer::GlobalInstance().RequestMgr()->PutRequest(req);
    }
    return nullptr;
}

void ExtentIOServiceImpl::Read(
        google::protobuf::RpcController *cntl_base,
        const pb::ReadRequest *request, pb::ReadResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

    Request *req = ExtentServer::GlobalInstance().RequestMgr()->GetRequest(
            RequestType::kTypeRead);
    if (req == nullptr) {
        LOG(ERROR) << "Couldn't get [Request]"
                   << ", extent_id: " << request->extent_id()
                   << ", offset: " << request->offset()
                   << ", size: " << request->size();
        response->mutable_status()->set_code(
                common::CYPRE_ES_GET_REQ_UNIT_FAIL);
        response->mutable_status()->set_message("get request unit fail");
        return;
    }

    done_guard.release();
    req->BeginTraceTime();
    req->SetOperationContext(
            cntl, const_cast<pb::ReadRequest *>(request), response, done);
    req->SetUserCallback(ReadDone);
    auto storage_engine = ExtentServer::GlobalInstance().StorageEngine();
    Status s = storage_engine->ProcessRequest(req);
    if (s.ok()) {
        return;
    }

    if (s.IsEmpty()) {
        req->SetResult(true);
        LOG(DEBUG) << "Read empty extent"
                   << ", extent_id: " << request->extent_id()
                   << ", offset: " << request->offset()
                   << ", size: " << request->size();
    } else {
        req->SetResult(false);
        LOG(ERROR) << "Couldn't process read request, " << s.ToString()
                   << ", extent_id: " << request->extent_id()
                   << ", offset: " << request->offset()
                   << ", size: " << request->size();
    }
    ReadDone((void *)req);
}

void *ExtentIOServiceImpl::WriteDone(void *arg) {
    Request *req = static_cast<Request *>(arg);
    if (req->FetchAndSubRef() != 1) {
        return nullptr;
    }
    auto &op_ctx = req->GetOperationContext();
    brpc::ClosureGuard done_guard(op_ctx.done);
    pb::WriteRequest *request = static_cast<pb::WriteRequest *>(op_ctx.request);
    pb::WriteResponse *response =
            static_cast<pb::WriteResponse *>(op_ctx.response);
    if (!req->Result()) {
        LOG(ERROR) << "Couldn't write extent to block device"
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        response->mutable_status()->set_code(
                common::CYPRE_ES_PROCESS_REQ_ERROR);
        response->mutable_status()->set_message("write io error");
    } else {
        response->mutable_status()->set_code(common::CYPRE_OK);
    }
    if (req->IOUnit() != nullptr) {
        req->GetIOMemMgr()->PutIOUnit(req->IOUnit());
    }
    req->EndTraceTime();
    ExtentServer::GlobalInstance().RequestMgr()->PutRequest(req);
    return nullptr;
}

void ExtentIOServiceImpl::Write(
        google::protobuf::RpcController *cntl_base,
        const pb::WriteRequest *request, pb::WriteResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

    Request *req = ExtentServer::GlobalInstance().RequestMgr()->GetRequest(
            RequestType::kTypeWrite);
    if (req == nullptr) {
        LOG(ERROR) << "Couldn't get [Request]"
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();

        response->mutable_status()->set_code(
                common::CYPRE_ES_GET_REQ_UNIT_FAIL);
        response->mutable_status()->set_message("get request unit fail");
        return;
    }

    // async
    done_guard.release();
    req->BeginTraceTime();
    req->SetOperationContext(
            cntl, const_cast<pb::WriteRequest *>(request), response, done);
    req->SetUserCallback(WriteDone);
    auto storage_engine = ExtentServer::GlobalInstance().StorageEngine();
    Status s = storage_engine->ProcessRequest(req);
    if (s.ok()) {
        return;
    }

    LOG(ERROR) << "Couldn't process write request, " << s.ToString()
               << ", extent_id:" << request->extent_id()
               << ", offset:" << request->offset()
               << ", size:" << request->size();
    req->SetResult(false);
    WriteDone((void *)req);
}

void *ExtentIOServiceImpl::DeleteDone(void *arg) {
    Request *req = static_cast<Request *>(arg);
    auto &op_ctx = req->GetOperationContext();
    brpc::ClosureGuard done_guard(op_ctx.done);

    pb::DeleteRequest *request =
            static_cast<pb::DeleteRequest *>(op_ctx.request);
    pb::DeleteResponse *response =
            static_cast<pb::DeleteResponse *>(op_ctx.response);
    if (!req->Result()) {
        LOG(ERROR) << "Couldn't delete extent"
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        response->mutable_status()->set_code(
                common::CYPRE_ES_PROCESS_REQ_ERROR);
        response->mutable_status()->set_message("delete error");
    } else {
        response->mutable_status()->set_code(common::CYPRE_OK);
    }
    if (req->IOUnit() != nullptr) {
        req->GetIOMemMgr()->PutIOUnit(req->IOUnit());
    }
    req->EndTraceTime();
    ExtentServer::GlobalInstance().RequestMgr()->PutRequest(req);
    return nullptr;
}

void ExtentIOServiceImpl::Delete(
        google::protobuf::RpcController *cntl_base,
        const pb::DeleteRequest *request, pb::DeleteResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

    Request *req = ExtentServer::GlobalInstance().RequestMgr()->GetRequest(
            RequestType::kTypeDelete);
    if (req == nullptr) {
        LOG(ERROR) << "Couldn't get [Request]"
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        response->mutable_status()->set_code(
                common::CYPRE_ES_GET_REQ_UNIT_FAIL);
        response->mutable_status()->set_message("get request unit fail");
        return;
    }

    // async
    done_guard.release();
    req->BeginTraceTime();
    req->SetOperationContext(
            cntl, const_cast<pb::DeleteRequest *>(request), response, done);
    req->SetUserCallback(DeleteDone);
    auto storage_engine = ExtentServer::GlobalInstance().StorageEngine();
    auto s = storage_engine->ProcessRequest(req);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't process delete request, " << s.ToString()
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        req->SetResult(false);
        DeleteDone((void *)req);
    }
}

void *ExtentIOServiceImpl::ReplicateDone(void *arg) {
    Request *req = static_cast<Request *>(arg);
    auto &op_ctx = req->GetOperationContext();
    brpc::ClosureGuard done_guard(op_ctx.done);

    pb::ReplicateRequest *request =
            static_cast<pb::ReplicateRequest *>(op_ctx.request);
    pb::ReplicateResponse *response =
            static_cast<pb::ReplicateResponse *>(op_ctx.response);
    if (!req->Result()) {
        LOG(ERROR) << "Couldn't replicate extent to block device"
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        response->mutable_status()->set_code(
                common::CYPRE_ES_PROCESS_REQ_ERROR);
        response->mutable_status()->set_message("replicate io error");
    } else {
        response->mutable_status()->set_code(common::CYPRE_OK);
    }
    if (req->IOUnit() != nullptr) {
        req->GetIOMemMgr()->PutIOUnit(req->IOUnit());
    }
    req->EndTraceTime();
    ExtentServer::GlobalInstance().RequestMgr()->PutRequest(req);
    return nullptr;
}

void ExtentIOServiceImpl::Replicate(
        google::protobuf::RpcController *cntl_base,
        const pb::ReplicateRequest *request, pb::ReplicateResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

    Request *req = ExtentServer::GlobalInstance().RequestMgr()->GetRequest(
            RequestType::kTypeReplicate);
    if (req == nullptr) {
        LOG(ERROR) << "Couldn't get [Request]"
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        response->mutable_status()->set_code(
                common::CYPRE_ES_GET_REQ_UNIT_FAIL);
        response->mutable_status()->set_message("get request unit fail");
        return;
    }

    // async
    done_guard.release();
    req->BeginTraceTime();
    req->SetOperationContext(
            cntl, const_cast<pb::ReplicateRequest *>(request), response, done);
    req->SetUserCallback(ReplicateDone);
    auto storage_engine = ExtentServer::GlobalInstance().StorageEngine();
    auto s = storage_engine->ProcessRequest(req);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't process replicate request, " << s.ToString()
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        req->SetResult(false);
        ReplicateDone((void *)req);
    }
}

void *ExtentIOServiceImpl::ScrubDone(void *arg) {
    Request *req = static_cast<Request *>(arg);
    auto &op_ctx = req->GetOperationContext();
    brpc::ClosureGuard done_guard(op_ctx.done);

    pb::ScrubRequest *request = static_cast<pb::ScrubRequest *>(op_ctx.request);
    pb::ScrubResponse *response =
            static_cast<pb::ScrubResponse *>(op_ctx.response);
    if (!req->Result()) {
        LOG(ERROR) << "Couldn't scrub extent"
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        response->mutable_status()->set_code(
                common::CYPRE_ES_PROCESS_REQ_ERROR);
        response->mutable_status()->set_message("scrub error");
    } else {
        if (req->IOUnit() != nullptr) {
            response->set_crc32(utils::Crc32::Checksum(
                    req->IOUnit()->data, req->IOUnit()->size));
        }
        response->mutable_status()->set_code(common::CYPRE_OK);
    }
    if (req->IOUnit() != nullptr) {
        req->GetIOMemMgr()->PutIOUnit(req->IOUnit());
    }
    req->EndTraceTime();
    ExtentServer::GlobalInstance().RequestMgr()->PutRequest(req);
    return nullptr;
}

void ExtentIOServiceImpl::Scrub(
        google::protobuf::RpcController *cntl_base,
        const pb::ScrubRequest *request, pb::ScrubResponse *response,
        google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

    Request *req = ExtentServer::GlobalInstance().RequestMgr()->GetRequest(
            RequestType::kTypeScrub);
    if (req == nullptr) {
        LOG(ERROR) << "Couldn't get [Request]"
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        response->mutable_status()->set_code(
                common::CYPRE_ES_GET_REQ_UNIT_FAIL);
        response->mutable_status()->set_message("get request unit fail");
        return;
    }

    // async
    done_guard.release();
    req->BeginTraceTime();
    req->SetOperationContext(
            cntl, const_cast<pb::ScrubRequest *>(request), response, done);
    req->SetUserCallback(ScrubDone);
    auto storage_engine = ExtentServer::GlobalInstance().StorageEngine();
    auto s = storage_engine->ProcessRequest(req);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't process scrub request, " << s.ToString()
                   << ", extent_id:" << request->extent_id()
                   << ", offset:" << request->offset()
                   << ", size:" << request->size();
        req->SetResult(false);
        ScrubDone((void *)req);
    }
}

}  // namespace extentserver
}  // namespace cyprestore
