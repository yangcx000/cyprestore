/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "extent_control_service.h"

#include <brpc/server.h>

#include <string>

#include "butil/iobuf.h"
#include "extentserver.h"

namespace cyprestore {
namespace extentserver {

void ExtentControlServiceImpl::ReclaimExtent(
        google::protobuf::RpcController *cntl_base,
        const pb::ReclaimExtentRequest *request,
        pb::ReclaimExtentResponse *response, google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

    Request *req = ExtentServer::GlobalInstance().RequestMgr()->GetRequest(
            RequestType::kTypeReclaimExtent);
    if (req == nullptr) {
        LOG(ERROR) << "Couldn't get [IORequest]"
                   << ", extent_id:" << request->extent_id();
        response->mutable_status()->set_code(
                common::CYPRE_ES_GET_REQ_UNIT_FAIL);
        response->mutable_status()->set_message("internal io error");
        return;
    }
    // sync
    auto storage_engine = ExtentServer::GlobalInstance().StorageEngine();
    req->SetOperationContext(
            cntl, const_cast<pb::ReclaimExtentRequest *>(request), response,
            done);
    auto s = storage_engine->ProcessRequest(req);
    ExtentServer::GlobalInstance().RequestMgr()->PutRequest(req);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't process reclaim extent request, "
                   << s.ToString() << ", extent_id:" << request->extent_id();
        response->mutable_status()->set_code(
                common::CYPRE_ES_PROCESS_REQ_ERROR);
        response->mutable_status()->set_message(s.ToString());
        return;
    }
    response->mutable_status()->set_code(common::CYPRE_OK);
}

}  // namespace extentserver
}  // namespace cyprestore
