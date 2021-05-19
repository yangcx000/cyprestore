/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#include "nbd_watch.h"

namespace cyprestore {
namespace nbd {

void NBDWatchContext::WatchImageSize() {
    if (started_ == false) {
        started_ = true;
        watch_thread_ = std::thread(&NBDWatchContext::WatchFunc, this);
    }
}

void NBDWatchContext::WatchFunc() {
    while (started_) {
        uint64_t new_size = blob_->GetBlobSize();
        if (new_size > 0 && new_size != current_size_) {
            LOG(NOTICE) << "image size changed, old size = " << current_size_
                      << ", new size = " << new_size;
            nbd_ctrl_->Resize(new_size);
            current_size_ = new_size;
        }

        sleeper_.WaitFor(std::chrono::seconds(5));
    }
}

void NBDWatchContext::StopWatch() {
    if (started_) {
        started_ = false;
        sleeper_.Interrupt();
        watch_thread_.join();

        started_ = false;
    }
}

}  // namespace nbd
}  // namespace cyprestore
