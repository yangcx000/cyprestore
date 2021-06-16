/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "spdk_mgr.h"

#include <butil/string_splitter.h>

#include "common/config.h"
#include "spdk/conf.h"   // spdk_conf_allocate
#include "spdk/env.h"    // spdk_unaffinitize_thread/spdk_env_opts
#include "spdk/event.h"  // SPDK_DEFAULT_RPC_ADDR
#include "spdk/rpc.h"
#include "spdk_internal/event.h"  // spdk_app_json_config_load/spdk_subsystem_init
#include "utils/set_cpu_affinity.h"

namespace cyprestore {
namespace extentserver {
void SpdkMgr::initBdevSubsystemDoneCallback(int rc, void *arg) {
    Context *ctx = static_cast<Context *>(arg);
    ctx->rc = initSpdkRpc(ctx->arg);
    ctx->done = true;
}

void SpdkMgr::initBdevSubsystemFunc(void *arg) {
    Context *ctx = static_cast<Context *>(arg);
    SpdkMgr *mgr = static_cast<SpdkMgr *>(ctx->arg);

    spdk_app_json_config_load(
            mgr->options_.json_config_file.c_str(), SPDK_DEFAULT_RPC_ADDR,
            initBdevSubsystemDoneCallback, arg, true);  // true: stop on errors
}

int SpdkMgr::initSpdkRpc(void *arg) {
    int rc;
    SpdkMgr *mgr = static_cast<SpdkMgr *>(arg);
    const char *listen_addr = mgr->options_.rpc_addr.c_str();

    if (listen_addr == nullptr) {
        LOG(ERROR)
                << "Couldn't initialize rpc service, the listen addr is null.";
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }

    if (!spdk_rpc_verify_methods()) {
        LOG(ERROR) << "Couldn't register rpc methods.";
        return common::CYPRE_ES_SPDK_RPC_REGISTER_ERROR;
    }

    /* Listen on the requested address */
    rc = spdk_rpc_listen(listen_addr);
    if (rc != 0) {
        LOG(ERROR) << "Couldn't listen on " << listen_addr;
        return common::CYPRE_ES_SPDK_RPC_LISTEN_ERROR;
    }

    spdk_rpc_set_state(SPDK_RPC_STARTUP);

    /* Register a poller to periodically check for RPCs */
    mgr->spdk_rpc_poller_ =
            SPDK_POLLER_REGISTER(doSpdkRpcPoll, NULL, SPDK_RPC_SELECT_INTERVAL);

    spdk_rpc_set_state(SPDK_RPC_RUNTIME);

    return 0;
}

Status SpdkMgr::initBdevSubsystem() {
    initSpdkThread();
    struct spdk_thread *init_thread = getOrCreateSpdkThread("init_spdk_thread");
    if (!init_thread)
        return Status(
                common::CYPRE_ES_SPDK_THREAD_CREATE_FAIL,
                "couldn't create spdk init thread");

    Context ctx;
    ctx.arg = this;
    /* Initialize the bdev layer */
    int rc = spdk_thread_send_msg(init_thread, initBdevSubsystemFunc, &ctx);
    if (rc != 0) return Status(rc, "couldn't send init bdev msg");

    do {
        spdk_thread_poll(init_thread, 0, 0);
    } while (!ctx.done);

    if (ctx.rc != 0) {
        return Status(ctx.rc, "couldn't init spdk bdev subsystem");
    }

    return Status();
}

Status SpdkMgr::initSpdkConf() {
    if (options_.rpc_addr.empty()) {
        return Status(
                common::CYPRE_ER_INVALID_ARGUMENT,
                "spdk rpc_addr path not found");
    }

    // use .json file
    if (options_.json_config_file.empty()) {
        return Status(
                common::CYPRE_ER_INVALID_ARGUMENT,
                "spdk json config file not found");
    }

    return Status();
}

Status SpdkMgr::initSpdkEnv() {
    struct spdk_env_opts opts;

    /* Initialize the environment library */
    spdk_env_opts_init(&opts);

    if (!options_.name.empty()) opts.name = options_.name.c_str();

    // Application CPU mask
    if (!options_.core_mask.empty())
        opts.core_mask = options_.core_mask.c_str();

    // Multi process mode
    // 使用同一shm_id的应用共享内存和NVMe设备
    // 第一个进程为primary process，余下的进程为secondary process
    if (options_.shm_id != -1) opts.shm_id = options_.shm_id;

    // DPDK使用的memory channel数量
    if (options_.mem_channel != -1) opts.mem_channel = options_.mem_channel;

    // Master (primary) core for DPDK
    if (options_.master_core != -1) opts.master_core = options_.master_core;

    // Reserve的hugepage内存大小,默认MB
    if (options_.mem_size != -1) opts.mem_size = options_.mem_size;

    if (options_.num_pci_addr != 0) opts.num_pci_addr = options_.num_pci_addr;

    opts.no_pci = options_.no_pci;
    opts.hugepage_single_segments = options_.hugepage_single_segments;
    // 完成初始化后unlink hugepage文件
    opts.unlink_hugepage = options_.unlink_hugepage;
    // allocate hugepages from a specific mount
    if (!options_.huge_dir.empty()) opts.hugedir = options_.huge_dir.c_str();

    if (spdk_env_init(&opts) < 0) {
        return Status(common::CYPRE_ES_SPDK_INIT_ERROR, "init spdk env failed");
    }

    return Status();
}

void SpdkMgr::initSpdkThread() {
    // TODO(yangchunxin3): bind core
    spdk_unaffinitize_thread();
    // Init thread lib before call thread_create()
    spdk_thread_lib_init(NULL, 0);
}

Status SpdkMgr::InitEnv() {
    Status s;
    if (status_ != kSpdkMgrInit) return s;

    s = initSpdkConf();
    if (!s.ok()) return s;

    s = initSpdkEnv();
    if (!s.ok()) return s;

    s = initBdevSubsystem();
    if (!s.ok()) return s;

    status_ = kSpdkMgrStarted;
    return s;
}

void SpdkMgr::finishBdevSubsystemDoneCallback(void *arg) {
    Context *ctx = static_cast<Context *>(arg);
    ctx->done = true;
    ctx->rc = 0;
}

void SpdkMgr::finishBdevSubsystemFunc(void *arg) {
    finishSpdkRpc(arg);
    spdk_subsystem_fini(finishBdevSubsystemDoneCallback, arg);
}

void SpdkMgr::finishSpdkRpc(void *arg) {
    Context *ctx = static_cast<Context *>(arg);
    struct spdk_poller *spdk_rpc_poller =
            static_cast<struct spdk_poller *>(ctx->arg);

    spdk_rpc_close();
    // The poller will get cleaned up in a subsequent call to
    // spdk_thread_poll().
    spdk_poller_unregister(&spdk_rpc_poller);
}

Status SpdkMgr::finishBdevSubsystem() {
    struct spdk_thread *th = getSpdkThread();
    if (!th) {
        return Status(
                common::CYPRE_ES_SPDK_THREAD_NULL,
                "the mgr's spkd thread is null");
    }

    Context ctx;
    ctx.arg = spdk_rpc_poller_;
    /* finish the bdev layer */
    int rc = spdk_thread_send_msg(th, finishBdevSubsystemFunc, &ctx);
    if (rc != 0) {
        return Status(rc, "couldn't send message to finish bdev subsystem");
    }

    do {
        spdk_thread_poll(th, 0, 0);
    } while (!ctx.done && !spdk_thread_is_idle(th));

    if (ctx.rc != 0) {
        return Status(ctx.rc, "couldn't finish bdev subsystem");
    }

    spdk_thread_exit(th);
    while (!spdk_thread_is_exited(th)) {
        spdk_thread_poll(th, 0, 0);
    }
    spdk_thread_destroy(th);

    return Status();
}

Status SpdkMgr::finishBdevEnv() {
    spdk_thread_lib_fini();

    return Status();
}

void SpdkMgr::finishIoChannelFunc(void *arg) {
    Context *ctx = static_cast<Context *>(arg);
    struct spdk_io_channel *io_channel =
            static_cast<spdk_io_channel *>(ctx->arg);

    spdk_put_io_channel(io_channel);
    ctx->done = true;
    ctx->rc = 0;
}

Status SpdkMgr::finishSpdkIoThread(
        struct spdk_thread *th, struct spdk_io_channel *io_channel) {
    if (!th) {
        return Status(
                common::CYPRE_ES_SPDK_THREAD_NULL,
                "the worker's spkd thread is null");
    }

    if (io_channel == nullptr) {
        LOG(WARNING) << "Warning: The worker's io channel is null.";
    } else {
        Context ctx;
        ctx.arg = io_channel;

        int rc = spdk_thread_send_msg(th, finishIoChannelFunc, &ctx);
        if (rc != 0) {
            return Status(rc, "couldn't send message to close io channel");
        }

        do {
            spdk_thread_poll(th, 0, 0);
        } while (!ctx.done && !spdk_thread_is_idle(th));

        if (ctx.rc != 0) {
            return Status(ctx.rc, "couldn't close io channel");
        }
    }

    spdk_thread_exit(th);
    while (!spdk_thread_is_exited(th)) {
        spdk_thread_poll(th, 0, 0);
    }
    spdk_thread_destroy(th);

    return Status();
}

Status SpdkMgr::CloseEnv() {
    Status s;
    if (status_ != kSpdkMgrStarted) return s;

    status_ = kSpdkMgrStopping;

    s = finishBdevSubsystem();
    if (!s.ok()) {
        return s;
    }

    s = finishBdevEnv();
    if (!s.ok()) {
        return s;
    }

    status_ = kSpdkMgrStopped;
    return s;
}

int SpdkMgr::doSpdkRpcPoll(void *arg) {
    spdk_rpc_accept();
    return -1;
}

struct spdk_thread *SpdkMgr::getOrCreateSpdkThread(const std::string &name) {
    struct spdk_thread *th = spdk_get_thread();
    if (th == nullptr) {
        th = spdk_thread_create(name.c_str(), nullptr);
        if (!th) return th;
        spdk_set_thread(th);
    }
    return th;
}

struct spdk_io_channel *SpdkMgr::getSpdkIoChannel() {
    if (getOrCreateSpdkThread("io_thread") == nullptr) return nullptr;
    return spdk_bdev_get_io_channel(handler_.desc);
}

struct spdk_thread *SpdkMgr::getSpdkThread() {
    return spdk_get_thread();
}

void SpdkMgr::openSpdkBdevFunc(void *arg) {
    Context *ctx = static_cast<Context *>(arg);
    SpdkMgr *mgr = static_cast<SpdkMgr *>(ctx->arg);
    mgr->handler_.bdev = spdk_bdev_get_by_name(ctx->dev_name.c_str());
    if (!mgr->handler_.bdev) {
        ctx->done = true;
        ctx->rc = common::CYPRE_ES_DISK_NOT_FOUND;
        return;
    }

    ctx->rc = spdk_bdev_open(
            mgr->handler_.bdev, true, NULL, NULL, &mgr->handler_.desc);
    ctx->done = true;
}

Status SpdkMgr::Open(const std::string &dev_name) {
    struct spdk_thread *th = getOrCreateSpdkThread(dev_name);
    if (!th)
        return Status(
                common::CYPRE_ES_SPDK_THREAD_CREATE_FAIL,
                "couldn't create bdev io thread");

    Context ctx(this, dev_name);
    int rc = spdk_thread_send_msg(th, openSpdkBdevFunc, &ctx);
    if (rc != 0) return Status(rc, "couldn't send open bdev msg");

    do {
        spdk_thread_poll(th, 0, 0);
    } while (!ctx.done);

    if (ctx.rc != 0) {
        return Status(ctx.rc, "couldn't open spdk bdev");
    }

    return Status();
}

void SpdkMgr::closeSpdkBdevFunc(void *arg) {
    Context *ctx = static_cast<Context *>(arg);
    SpdkMgr *mgr = static_cast<SpdkMgr *>(ctx->arg);

    if (!mgr->handler_.bdev || !mgr->handler_.desc) {
        ctx->done = true;
        ctx->rc = common::CYPRE_ES_DISK_NOT_FOUND;
        return;
    }

    spdk_bdev_close(mgr->handler_.desc);
    ctx->done = true;
    ctx->rc = 0;
}

Status SpdkMgr::Close(const std::string &dev_name) {
    struct spdk_thread *th = getOrCreateSpdkThread(dev_name);
    if (!th) {
        return Status(
                common::CYPRE_ES_SPDK_THREAD_CREATE_FAIL,
                "the mgr's spkd thread is null");
    }

    Context ctx(this, dev_name);
    int rc = spdk_thread_send_msg(th, closeSpdkBdevFunc, &ctx);
    if (rc != 0) {
        return Status(rc, "couldn't send message to close bdev");
    }

    do {
        spdk_thread_poll(th, 0, 0);
    } while (!ctx.done);

    if (ctx.rc != 0) {
        if (ctx.rc != common::CYPRE_ES_DISK_NOT_FOUND) {
            return Status(ctx.rc, "couldn't close bdev");
        }
        LOG(WARNING) << "Warning: bdev_desc is null, don't need to close it.";
    }

    return Status();
}

/*
Status SpdkMgr::Readv(struct iovec *iov, int iovcnt, uint64_t offset, uint64_t
len) { struct spdk_io_channel *io_channel = get_spdk_io_channel(); if
(io_channel == nullptr) return Status(Status::kIOError, "couldn't get io
channel");

    Context ctx;
    int rc = spdk_bdev_readv(handler_.desc, io_channel, iov, iovcnt, offset,
len, cb_io_completion, &ctx); if (rc != 0) return Status(Status::kIOError, "read
io error");

    struct spdk_thread *io_thread = get_spdk_thread();
    do {
        spdk_thread_poll(io_thread, 0, 0);
    } while (!ctx.done);

    if (ctx.rc != 0) return Status(Status::kIOError, "read io error");
    return Status();
}
*/
/*
Status SpdkMgr::Writev(struct iovec *iov, int iovcnt, uint64_t offset, uint64_t
len) { struct spdk_io_channel *io_channel = get_spdk_io_channel(); if
(io_channel == nullptr) return Status(Status::kIOError, "couldn't get io
channel");

    Context ctx;
    int rc = spdk_bdev_writev(handler_.desc, io_channel, iov, iovcnt, offset,
len, cb_io_completion, &ctx); if (rc != 0) return Status(Status::kIOError,
"write io error");

    struct spdk_thread *io_thread = get_spdk_thread();
    do {
        spdk_thread_poll(io_thread, 0, 0);
    } while (!ctx.done);

    if (ctx.rc != 0) return Status(Status::kIOError, "write io error");
    return Status();
}
*/

void *SpdkMgr::AllocIOMem(size_t size, size_t align) {
    return spdk_dma_zmalloc(size, align, nullptr);
}

void SpdkMgr::FreeIOMem(void *p) {
    spdk_dma_free(p);
}

uint32_t SpdkMgr::GetBlockSize() {
    return spdk_bdev_get_block_size(handler_.bdev);
}

uint64_t SpdkMgr::GetNumBlocks() {
    return spdk_bdev_get_num_blocks(handler_.bdev);
}

// Write unit size is required number of logical blocks to perform write
// operation on block device. Unit of write unit size is logical block and the
// minimum of write unit size is one. Write operations must be multiple of write
// unit size.
uint32_t SpdkMgr::GetWriteUnitSize() {
    return spdk_bdev_get_write_unit_size(handler_.bdev);
}

// Get minimum I/O buffer address alignment for a bdev.
size_t SpdkMgr::GetBufAlignSize() {
    return spdk_bdev_get_buf_align(handler_.bdev);
}

uint32_t SpdkMgr::GetMDSize() {
    return spdk_bdev_get_md_size(handler_.bdev);
}

uint32_t SpdkMgr::GetOptimalIOBoundary() {
    return spdk_bdev_get_optimal_io_boundary(handler_.bdev);
}

// Return I/O statistics for this channel.
void SpdkMgr::GetIOStat(struct spdk_bdev_io_stat *stat) {
    struct spdk_io_channel *io_channel = getSpdkIoChannel();
    if (io_channel == nullptr) return;

    spdk_bdev_get_io_stat(handler_.bdev, io_channel, stat);
}

void SpdkMgr::GetOpts(struct spdk_bdev_opts *opts) {
    spdk_bdev_get_opts(opts);
}

Status SpdkMgr::PeriodDeviceAdmin() {
    if (status_ != kSpdkMgrStarted) {
        return Status();
    }

    struct spdk_thread *th = getSpdkThread();
    if (th == nullptr) {
        return Status(
                common::CYPRE_ES_SPDK_THREAD_NULL,
                "the mgr's spkd thread is null");
    }

    if (spdk_rpc_poller_ == nullptr) {
        return Status(
                common::CYPRE_ES_SPDK_RPC_CLOSED, "spdk rpc poller is null");
    }

    if (!spdk_thread_is_exited(th)) {
        spdk_thread_poll(th, 0, 0);
    }

    return Status();
}

Status SpdkMgr::ProcessRequest(Request *req) {
    size_t count = task_queue_->Enqueue((void **)&req);
    if (count == 0)
        return Status(
                common::CYPRE_ES_RTE_RING_FULL, "couldn't submit request");
    return Status();
}

void SpdkMgr::getCoreMask(std::vector<int> &core_mask_vector) {
    std::string core_mask =
            GlobalConfig().extentserver().spdk_worker_core_mask;
    if (core_mask.empty()) { return; }
    butil::StringSplitter sp(core_mask.c_str(), ',');
    std::vector<std::string> result;
    for (; sp; ++sp) {
        result.push_back(sp.field());
    }
    if (result.empty()) { return; }
    for (auto &core : result) {
        core_mask_vector.push_back(std::stoi(core));
    }
}

Status SpdkMgr::StartWorkers() {
    // 初始化
    CypreRing *ring = new CypreRing(
            "spdk_request_ring", CypreRing::TYPE_MP_MC,
            GlobalConfig().extentserver().spdk_request_ring_size);
    if (!ring) {
        return Status(
                common::CYPRE_ER_OUT_OF_MEMORY, "couldn't create spdk_io_ring");
    }
    Status s = ring->Init();
    if (!s.ok()) return s;

    task_queue_.reset(ring);
    auto num_workers = GlobalConfig().extentserver().num_spdk_workers;
    workers_.resize(num_workers);

    // parse core mask
    std::vector<int> core_mask;
    getCoreMask(core_mask);
    bool set_affinity = true;
    if (core_mask.empty() || static_cast<int>(core_mask.size()) != num_workers) {
        LOG(INFO) << "Not bind cpu core"
                  << ", core mask is empty or not match worker num"
                  << ", core mask num :" << core_mask.size()
                  << ", worker num: " << num_workers;
        set_affinity = false;
    }

    for (int i = 0; i < num_workers; ++i) {
        workers_[i] = new SpdkWorker(this, task_queue_);
        int ret = pthread_create(
                workers_[i]->ThreadId(), NULL, SpdkWorker::SpdkWorkerFunc,
                (void *)workers_[i]);
        if (ret != 0) {
            return Status(
                    common::CYPRE_ES_PTHREAD_CREATE_ERROR,
                    "Couldn't create worker thread");
        }

        if (set_affinity) {
            if (utils::SetCpuAffinityUtil::BindCore(
                    *workers_[i]->ThreadId(), core_mask[i])) {
                LOG(INFO) << "Bind spdk worker: "
                          << *workers_[i]->ThreadId()
                          << " to core: " << core_mask[i]
                          << " success";
                continue;
            }
            LOG(ERROR) << "Bind spdk worker: "
                       << *workers_[i]->ThreadId()
                       << " to core: " << core_mask[i]
                       << " fail";
            return Status(common::CYPRE_ES_PTHREAD_BIND_CORE_ERROR,
                    "Couldn't bind pthread to cpu core");
        }
    }
    return Status();
}

Status SpdkMgr::StopWorkers() {
    // close spdk io channel and spdk thread
    int ret = 0, th_status = 0;
    void *worker_status;
    auto num_workers = GlobalConfig().extentserver().num_spdk_workers;
    for (int i = 0; i < num_workers; ++i) {
        workers_[i]->Stop();

        ret = pthread_join(*(workers_[i]->ThreadId()), &worker_status);
        th_status = *(int *)worker_status;
        if (ret != 0 || th_status != kSpdkWorkerStopped) {
            LOG(ERROR) << "Couldn't stop " << num_workers << " worker threads"
                       << " since pthread_join returns non-zero or the worker "
                          "status"
                       << " is not kSpdkWorkerStopped(" << kSpdkWorkerStopped
                       << ")."
                       << " The retcode: " << ret
                       << ", worker status: " << th_status;
            return Status(
                    common::CYPRE_ES_PTHREAD_JOIN_ERROR,
                    "couldn't stop worker thread");
        }
        delete workers_[i];
    }
    workers_.clear();
    return Status();
}

bool SpdkMgr::WriteCacheEnabled() {
    return spdk_bdev_has_write_cache(handler_.bdev);
}
}  // namespace extentserver
}  // namespace cyprestore
