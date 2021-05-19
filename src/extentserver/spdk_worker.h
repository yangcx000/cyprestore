/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_SPDK_WORKER_H
#define CYPRESTORE_EXTENTSERVER_SPDK_WORKER_H

#include <butil/macros.h>
#include <pthread.h>
#include <memory>

#include "common/cypre_ring.h"
#include "io_mem.h"
#include "request_context.h"
#include "spdk/bdev_module.h"  // spdk_bdev
#include "spdk/thread.h"  // spdk_io_channel


namespace cyprestore {
namespace extentserver {

class SpdkMgr;

enum SpdkWorkerStatus {
    kSpdkWorkerInit = 0,
    kSpdkWorkerStopping,
    kSpdkWorkerStopped,
};

class SpdkWorker {
 public:
    SpdkWorker(SpdkMgr *spdk_mgr, std::shared_ptr<CypreRing> &task_queue)
        : spdk_mgr_(spdk_mgr),
          task_queue_(task_queue),
          status_(kSpdkWorkerInit) {
    }
    ~SpdkWorker() = default;

    static void *SpdkWorkerFunc(void *arg);
    void Stop();

    pthread_t *ThreadId() {
        return &tid_;
    }

 private:
    static void worker_callback(struct spdk_bdev_io *io, bool success, void *arg);

    void initWorkerEnv();
    void run();

    void doRead(Request *req);
    void doWrite(Request *req);
    void doDelete(Request *req);

	pthread_t tid_;
	SpdkMgr *spdk_mgr_;
	std::shared_ptr<CypreRing> task_queue_;
	struct spdk_thread *io_thread_;
	struct spdk_io_channel *io_channel_;
    std::shared_ptr<IOMemMgr> iomem_mgr_;
    const int kBatchNums = 1000;
    volatile SpdkWorkerStatus status_;
};


}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_SPDK_WORKER_H
