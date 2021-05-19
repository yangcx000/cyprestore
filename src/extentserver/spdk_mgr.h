/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_SPDK_MGR_H
#define CYPRESTORE_EXTENTSERVER_SPDK_MGR_H

#include <pthread.h>

#include <string>
#include <vector>

#include "bthread/types.h"
#include "butil/macros.h"
#include "common/cypre_ring.h"
#include "common/status.h"
#include "request_context.h"
#include "spdk/bdev_module.h"      // spdk_bdev
#include "spdk/thread.h"           // spdk_io_channel
#include "spdk_internal/thread.h"  // spdk_thread
#include "spdk_worker.h"           // spdk_thread

namespace cyprestore {
namespace extentserver {

const int SPDK_RPC_SELECT_INTERVAL = 4000;  // 4ms

enum SpdkMgrStatus {
    kSpdkMgrInit = 0,
    kSpdkMgrStarted,
    kSpdkMgrStopping,
    kSpdkMgrStopped,
};

struct SpdkEnvOptions {
    int shm_id;
    int mem_channel;
    int mem_size;
    int master_core;
    size_t num_pci_addr;
    bool no_pci;
    bool hugepage_single_segments;
    bool unlink_hugepage;
    std::string core_mask;
    std::string huge_dir;
    std::string name;
    std::string json_config_file;
    std::string rpc_addr;
};

struct SpdkHandler {
    SpdkHandler() : bdev(nullptr), desc(nullptr) {}

    struct spdk_bdev *bdev;
    struct spdk_bdev_desc *desc;
};

using common::Status;

class SpdkMgr {
public:
    explicit SpdkMgr(const SpdkEnvOptions &options)
            : options_(options), status_(kSpdkMgrInit) {}

    ~SpdkMgr() = default;

    Status InitEnv();
    Status CloseEnv();

    Status Open(const std::string &dev_name);
    Status Close(const std::string &dev_name);  // close bdev and desc

    void *AllocIOMem(size_t size, size_t align = 0x1000);
    void FreeIOMem(void *p);

    uint32_t GetBlockSize();
    uint64_t GetNumBlocks();
    uint32_t GetWriteUnitSize();
    uint32_t GetMDSize();
    uint32_t GetOptimalIOBoundary();
    size_t GetBufAlignSize();

    void GetIOStat(struct spdk_bdev_io_stat *stat);
    void GetOpts(struct spdk_bdev_opts *opts);
    SpdkEnvOptions GetEnvOptions() const {
        return options_;
    }

    Status PeriodDeviceAdmin();
    Status ProcessRequest(Request *req);

    Status StartWorkers();
    Status StopWorkers();

    bool WriteCacheEnabled();

private:
    friend class SpdkWorker;
    static void initBdevSubsystemDoneCallback(int rc, void *arg);
    static void initBdevSubsystemFunc(void *arg);
    static int initSpdkRpc(void *arg);

    Status initBdevSubsystem();
    Status initSpdkConf();
    Status initSpdkEnv();
    void initSpdkThread();

    static void finishBdevSubsystemDoneCallback(void *arg);
    static void finishBdevSubsystemFunc(void *arg);
    static void finishSpdkRpc(void *arg);
    static void finishIoChannelFunc(void *arg);

    Status finishBdevSubsystem();
    Status finishBdevEnv();
    Status finishSpdkIoThread(
            struct spdk_thread *th, struct spdk_io_channel *io_channel);

    static int doSpdkRpcPoll(void *arg);

    struct spdk_thread *getOrCreateSpdkThread(const std::string &name);
    struct spdk_io_channel *getSpdkIoChannel();
    struct spdk_thread *getSpdkThread();

    static void openSpdkBdevFunc(void *arg);
    static void closeSpdkBdevFunc(void *arg);

    void getCoreMask(std::vector<int> &core_mask_vector);

    struct Context {
        Context() : done(false), rc(0), arg(nullptr) {}

        Context(void *arg_, const std::string &dev_name_)
                : done(false), rc(0), arg(arg_), dev_name(dev_name_) {}

        bool done;
        int rc;
        void *arg;
        std::string dev_name;
    };

    SpdkEnvOptions options_;
    SpdkHandler handler_;
    std::shared_ptr<CypreRing> task_queue_;
    std::vector<SpdkWorker *> workers_;
    struct spdk_poller *spdk_rpc_poller_;
    volatile SpdkMgrStatus status_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_SPDK_MGR_H
