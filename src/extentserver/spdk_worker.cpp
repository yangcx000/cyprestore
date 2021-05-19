/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "spdk_worker.h"

#include "bthread/bthread.h"
#include "spdk_mgr.h"

namespace cyprestore {
namespace extentserver {

void *SpdkWorker::SpdkWorkerFunc(void *arg) {
    SpdkWorker *worker = static_cast<SpdkWorker *>(arg);
    worker->run();
    return nullptr;
}

void SpdkWorker::Stop() {
    status_ = kSpdkWorkerStopping;
}

void SpdkWorker::worker_callback(
        struct spdk_bdev_io *io, bool success, void *arg) {
    Request *req = static_cast<Request *>(arg);
    req->SetResult(success);
    bthread_t th;
    while (bthread_start_background(&th, nullptr, req->UserCallback(), arg)
           != 0) {
        LOG(FATAL) << "Fail to start user callback";
    }
    // Complete the I/O
    spdk_bdev_free_io(io);
}

void SpdkWorker::initWorkerEnv() {
    io_thread_ = spdk_mgr_->getOrCreateSpdkThread("spdk_io_thread");
    assert(io_thread_ != nullptr && "couldn't create spdk thread");

    io_channel_ = spdk_mgr_->getSpdkIoChannel();
    assert(io_channel_ != nullptr && "couldn't create spdk io channel");

    iomem_mgr_.reset(new IOMemMgr(CypreRing::CypreRingType::TYPE_MP_SC));
    assert(iomem_mgr_ != nullptr && "couldn't alloc IOMemMgr");
    auto status = iomem_mgr_->Init();
    assert(status.ok() && "Init io mem manager failed");
}

void SpdkWorker::run() {
    initWorkerEnv();

    pthread_setname_np(tid_, "spdk_worker");
    LOG(INFO) << "Start spdk worker: " << tid_;
    Request *reqs[kBatchNums];
    Status s;

    // TODO(feifei5) how to handle this loop, when to stop?
    while (true) {
        spdk_thread_poll(io_thread_, 0, 0);
        size_t count = task_queue_->DequeueBurst((void **)reqs, kBatchNums);
        if (count == 0) {
            if (status_ == kSpdkWorkerStopping) {
                break;
            }
            continue;
        }

        for (size_t i = 0; i < count;) {
            io_u *io = nullptr;
            // if can't get io unit, infinite retry
            s = iomem_mgr_->GetIOUnitBulk(reqs[i]->Size(), &io);
            if (!s.ok()) {
                LOG(ERROR) << "Couldn't get io unit " << s.ToString();
                continue;
            }
            reqs[i]->SetIOMemMgr(iomem_mgr_);
            reqs[i]->SetIOUnit(io);

            switch (reqs[i]->GetRequestType()) {
                case RequestType::kTypeRead:
                case RequestType::kTypeScrub:
                    doRead(reqs[i]);
                    break;
                case RequestType::kTypeWrite:
                case RequestType::kTypeReplicate:
                    doWrite(reqs[i]);
                    break;
                // here no need IOUnit, but for code unification,
                // do not treat special
                case RequestType::kTypeDelete:
                    doDelete(reqs[i]);
                    break;
                default:
                    LOG(ERROR) << "Invalid cmd type: "
                               << reqs[i]->GetRequestType();
                    assert(false);
                    continue;
            }
            ++i;
        }
    }

    s = spdk_mgr_->finishSpdkIoThread(io_thread_, io_channel_);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't close io channel or spdk thread. Error: "
                   << s.ToString();
    } else {
        status_ = kSpdkWorkerStopped;
    }

    LOG(INFO) << "Worker: pthread exits with status(" << status_ << ")";
    pthread_exit((void *)(&status_));
}

void SpdkWorker::doRead(Request *req) {
    int rc = spdk_bdev_read(
            spdk_mgr_->handler_.desc, io_channel_, req->IOUnit()->data,
            req->PhysicalOffset(), req->Size(), worker_callback, (void *)req);
    if (rc == 0) {
        return;
    }

    LOG(ERROR) << "bdev read error, rc: " << rc
               << ", physical offset: " << req->PhysicalOffset()
               << ", size: " << req->Size();
    req->SetResult(false);
    req->UserCallback()(req);
}

void SpdkWorker::doWrite(Request *req) {
    auto cntl = req->GetOperationContext().cntl;
    cntl->request_attachment().copy_to(req->IOUnit()->data, req->Size(), 0);
    int rc = spdk_bdev_write(
            spdk_mgr_->handler_.desc, io_channel_, req->IOUnit()->data,
            req->PhysicalOffset(), req->Size(), worker_callback, (void *)req);
    if (rc == 0) {
        return;
    }

    LOG(ERROR) << "bdev write error, rc: " << rc
               << ", physical offset: " << req->PhysicalOffset()
               << ", size: " << req->Size();
    req->SetResult(false);
    req->UserCallback()(req);
}

void SpdkWorker::doDelete(Request *req) {
    int rc = spdk_bdev_write_zeroes(
            spdk_mgr_->handler_.desc, io_channel_, req->PhysicalOffset(),
            req->Size(), worker_callback, (void *)req);
    if (rc == 0) {
        return;
    }

    LOG(ERROR) << "bdev write zeros error, rc: " << rc
               << ", physical offset: " << req->PhysicalOffset()
               << ", size: " << req->Size();
    req->SetResult(false);
    req->UserCallback()(req);
}

}  // namespace extentserver
}  // namespace cyprestore
