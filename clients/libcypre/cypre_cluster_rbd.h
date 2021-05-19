/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 *
 */

#ifndef CYPRESTORE_CLIENTS_CYPRE_CLUSTER_RBD_H_
#define CYPRESTORE_CLIENTS_CYPRE_CLUSTER_RBD_H_

#include <brpc/channel.h>

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common/connection_pool.h"
#include "common/extent_router.h"
#include "libcypre.h"
#include "stream/rbd_stream_handle.h"

namespace cyprestore {
namespace clients {

class BrpcSenderWorker;
class CypreClusterRBD : public CypreRBD {
public:
    CypreClusterRBD() : em_channel_(nullptr), brpc_sender_(NULL) {}
    virtual ~CypreClusterRBD() {
        Finalize();
    }

    virtual int Init(const CypreRBDOptions &opts);
    virtual int Finalize();
    virtual int Open(const std::string &blob_id, RBDStreamHandlePtr &handle);
    virtual int Close(RBDStreamHandlePtr &handle);

private:
	int setBrpcWorkerThreads(int num_threads);

    CypreRBDOptions options_;
    brpc::Channel *em_channel_;
    common::ExtentRouterMgrPtr extent_router_mgr_;
    std::list<RBDStreamHandlePtr> stream_table_;
    std::mutex lock_;
    BrpcSenderWorker *brpc_sender_;
};

}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_CYPRE_CLUSTER_RBD_H_
