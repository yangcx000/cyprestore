
/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 */

#ifndef CYPRESTORE_CLIENTS_CONCURRENCY_IO_CONCURRENCY_H_
#define CYPRESTORE_CLIENTS_CONCURRENCY_IO_CONCURRENCY_H_

#include <deque>
#include <list>
#include <map>
#include <mutex>

#include "stream/user_request.h"

namespace cyprestore {
namespace clients {

// Request Concurrency Controller, base on io range check.
// if a request overlapped with any in-flight requests, it will be queued
// in a pending list.
// NOTE: when one request returned, we only check the front element of the
// pending list.

// TODO(zhangliang): need refine, remove from sdk later
class IoShard {
public:
    IoShard() {}
    ~IoShard() {}

    std::mutex *GetLock() {
        return &lock_;
    }
    bool Check(uint64_t offset, uint32_t len);
    bool Insert(uint64_t offset, uint32_t len);
    void Erase(uint64_t offset, uint32_t len);
    void Clear();

private:
    std::map<uint64_t, uint64_t> doingMap_;
    std::mutex lock_;
};

class IoConcurrency {
public:
    IoConcurrency();
    ~IoConcurrency();

    bool SendWriteRequest(UserWriteRequest *req, bool enqueIfPending);
    bool SendReadRequest(UserReadRequest *req, bool enqueIfPending);
    // return true if @ioReq is a valid pending request
    // @ioReq, the next pending request to be scheduled
    bool OnWriteDone(uint64_t offset, uint32_t len, UserIoRequest **ioReq);
    // return true if @ioReq is a valid pending request
    // @ioReq, the next pending request to be scheduled
    bool OnReadDone(uint64_t offset, uint32_t len, UserIoRequest **ioReq);

    void GetStats(uint64_t &doingSize, uint64_t &pendingSize);

    class Iterator {
    public:
        Iterator(IoConcurrency *io) : io_(io) {
            io->lock_.lock();
            itr_ = io_->pendingList_.begin();
        }
        ~Iterator() {
            io_->lock_.unlock();
        }
        inline bool hasNext() const {
            return itr_ != io_->pendingList_.end();
        }
        inline UserIoRequest *Next() {
            return (*itr_++);
        }
        // we can't clear pending list directly. when clear, do the following:
        // 1 iterator the pending list, and call user's callback
        // 2 call Clear()
        void Clear();

    private:
        Iterator(const Iterator &) = delete;
        Iterator operator=(const Iterator &) = delete;

        IoConcurrency *io_;
        std::deque<UserIoRequest *>::iterator itr_;
    };
    Iterator *NewIter() {
        return new Iterator(this);
    }

private:
    IoConcurrency(const IoConcurrency &) = delete;
    IoConcurrency operator=(const IoConcurrency &) = delete;

    bool check(uint64_t offset, uint32_t len);
    void remove(uint64_t offset, uint32_t len);
    bool schedulePending(UserIoRequest **ioReq);

    IoShard *doingList_;
    std::deque<UserIoRequest *> pendingList_;
    std::atomic<uint64_t> doingSize_;
    std::atomic<uint64_t> pendingSize_;
    std::mutex lock_;
    friend class Iterator;
};

}  // namespace clients
}  // namespace cyprestore
#endif  // CYPRESTORE_CLIENTS_CONCURRENCY_IO_CONCURRENCY_H_
