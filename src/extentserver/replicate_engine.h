/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTSERVER_REPLICATOR_H_
#define CYPRESTORE_EXTENTSERVER_REPLICATOR_H_

#include <memory>

#include "common/connection_pool.h"
#include "common/extent_router.h"
#include "common/status.h"
#include "request_context.h"
#include "pb/extent_io.pb.h"

namespace cyprestore {
namespace extentserver {

class ReplicateEngine;
typedef std::shared_ptr<ReplicateEngine> ReplicateEnginePtr;

using common::Status;

class ReplicateEngine {
public:
    ReplicateEngine(const common::ExtentRouterMgrPtr router_mgr)
            : router_mgr_(router_mgr) {
        conn_pool_.reset(new common::ConnectionPool());
    }
    ~ReplicateEngine() = default;

    static void HandleResponse(brpc::Controller *cntl, pb::ReplicateResponse *resp, void *arg);
    Status Send(Request *req);

private:
    void sendOneReplicate(Request *req, common::ConnectionPtr conn);

    common::ExtentRouterMgrPtr router_mgr_;
    std::unique_ptr<common::ConnectionPool> conn_pool_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_REPLICATOR_H_
