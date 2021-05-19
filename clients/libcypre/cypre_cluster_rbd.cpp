/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 *
 */

#include "cypre_cluster_rbd.h"

#include <bthread/bthread.h>
#include <butil/time.h>
#include <bvar/bvar.h>

#include <cassert>
#include <memory>
#include <utility>

#include "common/error_code.h"
#include "extentmanager/pb/resource.pb.h"
#include "extentserver/pb/extent_io.pb.h"
#include "stream/brpc_es_wrapper.h"
#include "stream/rbd_stream_handle_impl.h"

namespace cyprestore {
namespace clients {

int CypreClusterRBD::setBrpcWorkerThreads(int num_threads) {
	// TODO(yangchunxin3): add cpu affinity
	int ret = bthread_set_concurrency_and_cpu_affinity(num_threads, "");
    if (ret != 0) {
        LOG(ERROR) << "Couldn't set brpc worker threads and cpu affinity"
				   << ", num_threads:" << num_threads
                   << ", ret: " << ret;
        return -1;
    }
	return 0;
}

int CypreClusterRBD::Init(const CypreRBDOptions &opts) {
	if (setBrpcWorkerThreads(opts.brpc_worker_thread_num) != 0) {
		return -1;
	}

    options_ = opts;
    std::string endpoint =
            options_.em_ip + ":" + std::to_string(options_.em_port);
    em_channel_ = new brpc::Channel();
    if (em_channel_->Init(endpoint.c_str(), NULL) != 0) {
        LOG(ERROR) << "Couldn't connect to extentmanager"
                   << ", em_ip:" << options_.em_ip
                   << ", em_port:" << options_.em_port;
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }

    // for brpc backgroud sender
    extent_router_mgr_.reset(new common::ExtentRouterMgr(em_channel_));
    int rv = BrpcEsWrapper::StartSenderWorker(
            opts.brpc_sender_thread, opts.brpc_sender_ring_power,
            opts.brpc_sender_thread_cpu_affinity, &brpc_sender_);
    if (rv != common::CYPRE_OK) {
        LOG(ERROR) << "BrpcEsWrapper::StartSenderWorker Failed:" << rv;
    } else {
        LOG(INFO) << "BrpcEsWrapper::StartSenderWorker ok.";
    }
    return rv;
}

int CypreClusterRBD::Open(
        const std::string &blob_id, RBDStreamHandlePtr &handle) {
    brpc::Controller cntl;
    extentmanager::pb::ResourceService_Stub stub(em_channel_);
    extentmanager::pb::QueryBlobRequest request;
    extentmanager::pb::QueryBlobResponse response;

    request.set_blob_id(blob_id);

    stub.QueryBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Couldn't send query blob request, " << cntl.ErrorText()
                   << ", blob_id:" << blob_id;
        return common::CYPRE_ER_NET_ERROR;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Couldn't query blob info, "
                   << response.status().message() << ", blob_id:" << blob_id;
        return response.status().code();  // server's error code
    }

    assert((blob_id == response.blob().id()) && "info inconsistency");

    RBDStreamOptions sopts;
    sopts.blob_id = response.blob().id();
    sopts.blob_name = response.blob().name();
    sopts.blob_size = response.blob().size();
    sopts.extent_size = response.extent_size();

    sopts.pool_id = response.blob().pool_id();
    sopts.user_id = response.blob().user_id();

    sopts.conn_pool.reset(new common::ConnectionPool2());
    sopts.extent_router_mgr = extent_router_mgr_;
    sopts.brpc_sender = brpc_sender_;
    RBDStreamHandlePtr streamHandle(new RBDStreamHandleImpl(sopts));
    int rv = streamHandle->Init();
    if (rv != common::CYPRE_OK) {
        LOG(ERROR) << "Init StreamHandle Failed:" << rv
                   << ", blob_id:" << blob_id;
        return rv;
    }
    rv = streamHandle->SetExtentIoProto(options_.proto);
    if (rv != common::CYPRE_OK) {
        LOG(ERROR) << "Set StreamHandle Proto Failed:" << rv
                   << ", blob_id:" << blob_id;
        return rv;
    }
    handle = streamHandle;
    LOG(WARNING) << "Open blob Ok, blob_id:" << sopts.blob_id
                 << ", name:" << sopts.blob_name << ", size:" << sopts.blob_size
                 << ", extent_size:" << sopts.extent_size;

    std::lock_guard<std::mutex> lock(lock_);
    stream_table_.push_back(streamHandle);
    return common::CYPRE_OK;
}

int CypreClusterRBD::Close(RBDStreamHandlePtr &streamHandle) {
    int rv = common::CYPRE_OK;
    if (streamHandle.get() != nullptr) {
        rv = streamHandle->Close();
        LOG(WARNING) << "Close StreamHandle return:" << rv
                     << ", blob_id:" << streamHandle->GetDeviceId();
    }
    std::lock_guard<std::mutex> lock(lock_);
    for (auto itr = stream_table_.begin(); itr != stream_table_.end(); ++itr) {
        const RBDStreamHandlePtr handle = *itr;
        if (handle.get() == streamHandle.get()) {
            stream_table_.erase(itr);
            break;
        }
    }
    streamHandle.reset();
    return rv;
}

int CypreClusterRBD::Finalize() {
    int rv = common::CYPRE_OK;
    extent_router_mgr_.reset();
    delete em_channel_;
    em_channel_ = NULL;
    BrpcEsWrapper::StopSenderWorker(brpc_sender_);
    brpc_sender_ = NULL;
    std::lock_guard<std::mutex> lock(lock_);
    size_t count = stream_table_.size();
    for (auto itr = stream_table_.begin(); itr != stream_table_.end(); ++itr) {
        RBDStreamHandlePtr handle = *itr;
        int rc = rv = handle->Close();
        LOG(WARNING) << "Close StreamHandle return:" << rv
                     << ", blob_id:" << handle->GetDeviceId();
        if (rc != common::CYPRE_OK) {
            rv = rc;
        }
        handle.reset();
    }
    stream_table_.clear();
    LOG(WARNING) << " CypreRBD Destroyed, handles:" << count
                 << ", return:" << rv;
    return rv;
}

}  // namespace clients
}  // namespace cyprestore
