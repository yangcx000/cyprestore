
/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 */

#ifndef CYPRESTORE_CLIENTS_CONCURRENCY_IO_DISPATCHER_H_
#define CYPRESTORE_CLIENTS_CONCURRENCY_IO_DISPATCHER_H_

#include <deque>
#include <list>
#include <map>
#include <mutex>

#include "stream/user_request.h"

namespace cyprestore {
namespace clients {

// IoDispatcher, based on flow control
class IoDispatcher {
public:
    IoDispatcher();
    ~IoDispatcher();

    UserRequest *Dispatch(UserRequest *req, bool sync);
    // @ureq, the finished user request
    // @nextReq, the next pending request to be scheduled
    bool OnUserRequestDone(UserRequest *ureq, UserRequest **nextReq);

    void GetStats(uint64_t &doingSize, uint64_t &pendingSize);

    class Iterator {
    public:
        Iterator(IoDispatcher *io) : io_(io) {
            io->lock_.lock();
            itr_ = io_->pendingList_.begin();
        }
        ~Iterator() {
            io_->lock_.unlock();
        }
        inline bool hasNext() const {
            return itr_ != io_->pendingList_.end();
        }
        inline UserRequest *Next() {
            return (*itr_++);
        }
        // we can't clear pending list directly. when clear, do the following:
        // 1 iterator the pending list, and call user's callback
        // 2 call Clear()
        void Clear() {
            io_->pendingList_.clear();
            itr_ = io_->pendingList_.begin();
        }

    private:
        Iterator(const Iterator &) = delete;
        Iterator operator=(const Iterator &) = delete;

        IoDispatcher *io_;
        std::deque<UserRequest *>::iterator itr_;
    };
    Iterator *NewIter() {
        return new Iterator(this);
    }

private:
    IoDispatcher(const IoDispatcher &) = delete;
    IoDispatcher operator=(const IoDispatcher &) = delete;

    bool schedulePending(UserRequest **nextReq);

    std::deque<UserRequest *> pendingList_;
    uint64_t doingSize_;
    uint64_t pendingSize_;
    std::mutex lock_;
    friend class Iterator;
};

}  // namespace clients
}  // namespace cyprestore
#endif  // CYPRESTORE_CLIENTS_CONCURRENCY_IO_CONCURRENCY_H_
