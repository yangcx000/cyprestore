
/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 */
#include "concurrency/io_concurrency.h"

#include <assert.h>

namespace cyprestore {
namespace clients {

class BatchLockGuard {
public:
    enum { kSize = 8 };
    BatchLockGuard() : nsize_(0) {}
    ~BatchLockGuard() {
        for (int i = 0; i < nsize_; i++) {
            locks_[i]->unlock();
        }
        nsize_ = 0;
    }
    inline void Push(std::mutex *lock) {
        if (nsize_ < kSize - 1) {
            lock->lock();
            locks_[nsize_++] = lock;
        }
    }

private:
    int nsize_;
    std::mutex *locks_[kSize];
};

bool IoShard::Check(uint64_t offset, uint32_t len) {
    if (doingMap_.empty()) {
        return true;
    }
    uint64_t left_max = 0;
    auto itr = doingMap_.lower_bound(offset);
    // check right overlap
    if (itr != doingMap_.end()) {
        if (itr->first < offset + len) {
            return false;
        }
        // prepare for left check
        if (itr-- != doingMap_.begin()) {
            left_max = itr->second;
        }
    } else {
        left_max = doingMap_.rbegin()->second;
    }
    // check left overlap
    if (left_max > offset) {
        return false;
    }
    return true;
}

bool IoShard::Insert(uint64_t offset, uint32_t len) {
    if (Check(offset, len)) {
        doingMap_[offset] = offset + len;
        return true;
    }
    return false;
}

void IoShard::Erase(uint64_t offset, uint32_t len) {
    const uint64_t end = offset + len;
    (void)(end);  // for release warning
    std::lock_guard<std::mutex> lock(lock_);
    auto itr = doingMap_.find(offset);
    if (itr != doingMap_.end()) {
        assert(end == itr->second);
        doingMap_.erase(itr);
    } else {
        assert(false);
    }
}

void IoShard::Clear() {
    std::lock_guard<std::mutex> lock(lock_);
    doingMap_.clear();
}

static const int IO_SHARD_SIZE = 4096;
static const int IO_SHARD_SIZE_MASK = (IO_SHARD_SIZE - 1);
static const int IO_SPLIT_SIZE = 16 * 1024 * 1024;
static const int IO_SPLIT_SIZE_SHIFT = 25;

void IoConcurrency::Iterator::Clear() {
    io_->pendingList_.clear();
    for (int i = 0; i < IO_SHARD_SIZE; i++) {
        io_->doingList_[i].Clear();
    }
    itr_ = io_->pendingList_.begin();
    io_->doingSize_ = io_->pendingSize_ = 0;
}

IoConcurrency::IoConcurrency() {
    doingSize_ = 0;
    pendingSize_ = 0;
    doingList_ = new IoShard[IO_SHARD_SIZE];
}

IoConcurrency::~IoConcurrency() {
    lock_.lock();
    delete[] doingList_;
    pendingList_.clear();
    lock_.unlock();
}

bool IoConcurrency::SendWriteRequest(
        UserWriteRequest *req, bool enqueIfPending) {
    if (!check(req->logic_offset, req->logic_len)) {
        if (enqueIfPending) {
            pendingSize_++;
            std::lock_guard<std::mutex> lock(lock_);
            pendingList_.push_back(req);
        }
        return false;
    }
    doingSize_++;
    return true;
}

bool IoConcurrency::SendReadRequest(UserReadRequest *req, bool enqueIfPending) {
    if (!check(req->logic_offset, req->logic_len)) {
        if (enqueIfPending) {
            pendingSize_++;
            std::lock_guard<std::mutex> lock(lock_);
            pendingList_.push_back(req);
        }
        return false;
    }
    doingSize_++;
    return true;
}

bool IoConcurrency::OnWriteDone(
        uint64_t offset, uint32_t len, UserIoRequest **ioReq) {
    doingSize_--;
    remove(offset, len);
    return schedulePending(ioReq);
}

bool IoConcurrency::OnReadDone(
        uint64_t offset, uint32_t len, UserIoRequest **ioReq) {
    doingSize_--;
    remove(offset, len);
    return schedulePending(ioReq);
}

void IoConcurrency::GetStats(uint64_t &doingSize, uint64_t &pendingSize) {
    doingSize = doingSize_;
    pendingSize = pendingSize_;
}

bool IoConcurrency::schedulePending(UserIoRequest **ioReq) {
    if (pendingSize_ == 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(lock_);
    if (pendingList_.empty()) {
        return false;
    }
    UserIoRequest *req = pendingList_.front();
    uint64_t offset = req->logic_offset;
    uint32_t len = req->logic_len;
    if (!check(offset, len)) {
        return false;
    }
    *ioReq = req;
    pendingList_.pop_front();
    pendingSize_--;
    return true;
}

bool IoConcurrency::check(uint64_t offset, uint32_t len) {
    if (len < 1) return true;
    const int idxs = (offset >> IO_SPLIT_SIZE_SHIFT) & IO_SHARD_SIZE_MASK;
    const int idxe =
            ((offset + len - 1) >> IO_SPLIT_SIZE_SHIFT) & IO_SHARD_SIZE_MASK;
    int idx = idxs - 1;
    BatchLockGuard guard;
    do {
        if (++idx == IO_SHARD_SIZE) {
            idx = 0;
        }
        guard.Push(doingList_[idx].GetLock());
    } while (idx != idxe);
    for (idx = idxe; idx != idxs; idx--) {
        if (idx < 0) {
            idx = IO_SHARD_SIZE - 1;
        }
        if (!doingList_[idx].Check(offset, len)) {
            return false;
        }
    }
    return doingList_[idxs].Insert(offset, len);
}

void IoConcurrency::remove(uint64_t offset, uint32_t len) {
    const int idxs = (offset >> IO_SPLIT_SIZE_SHIFT) & IO_SHARD_SIZE_MASK;
    doingList_[idxs].Erase(offset, len);
}

}  // namespace clients
}  // namespace cyprestore
