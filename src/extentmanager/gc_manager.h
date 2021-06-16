/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTMANAGER_GC_MANAGER_H_
#define CYPRESTORE_EXTENTMANAGER_GC_MANAGER_H_

#include "common/connection_pool.h"
#include "common/pb/types.pb.h"
#include "common/status.h"
#include "extentserver/pb/extent_control.pb.h"
#include "extentserver/pb/extent_io.pb.h"

namespace cyprestore {
namespace extentmanager {

using common::Status;

class GcManager {
public:
    GcManager() = default;
    ~GcManager() = default;

    int Init();
    static void GCFunc(void *arg);

private:
    void gc();
    bool timeValid();
    void deleteUserBlobs();
    void deleteBlobs();
    void deletePools();
    Status deleteBlobs(
            const std::string &pool_id, const std::vector<std::string> &blobs,
            bool delete_remote);
    Status deleteOneBlob(
            const std::string &pool_id, const std::string &blob_id,
            bool delete_remote);
    Status deleteRemoteExtent(const std::string &extent_id);
    Status deleteOneBlock(
            const std::string &extent_id, uint64_t offset,
            const common::pb::ExtentRouter &router);
    Status reclaimExtent(
            const std::string &extent_id,
            const common::pb::ExtentRouter &router);
    Status
    getRouter(const std::string &extent_id, common::pb::ExtentRouter *router);
    int getConns(
            const common::pb::ExtentRouter &router,
            std::vector<common::ConnectionPtr> *conns);

    volatile bool gcing_;
    const uint64_t block_size_ = 4 << 10;  // 4k
    std::unique_ptr<common::ConnectionPool> conn_pool_;

    struct ReclaimContext {
        extentserver::pb::ReclaimExtentResponse response;
        brpc::Controller cntl;
    };

    struct DeleteContext {
        extentserver::pb::DeleteResponse response;
        brpc::Controller cntl;
    };
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_GC_MANAGER_H_
