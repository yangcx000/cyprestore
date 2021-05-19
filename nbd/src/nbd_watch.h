/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_NBDWATCHCONTEXT_H_
#define NBD_SRC_NBDWATCHCONTEXT_H_

#include <atomic>
#include <memory>
#include <thread>

#include "blob_instance.h"
#include "interruptible_sleeper.h"
#include "nbd_ctrl.h"

namespace cyprestore {
namespace nbd {

// 定期获取卷大小
// 卷大小发生变化后，通知NBDController
class NBDWatchContext {
public:
    NBDWatchContext(
            NBDControllerPtr nbdCtrl, std::shared_ptr<BlobInstance> image,
            uint64_t currentSize)
            : nbd_ctrl_(nbdCtrl), blob_(image), current_size_(currentSize),
              started_(false) {}

    ~NBDWatchContext() {
        StopWatch();
    }

    /**
     * @brief 开始定期获取卷大小任务
     */
    void WatchImageSize();

    /**
     * @brief 停止任务
     */
    void StopWatch();

private:
    void WatchFunc();

    // nbd控制器
    NBDControllerPtr nbd_ctrl_;

    // 当前卷实例
    std::shared_ptr<BlobInstance> blob_;

    // 当前卷大小
    uint64_t current_size_;

    // 任务是否开始
    std::atomic<bool> started_;

    // 任务线程
    std::thread watch_thread_;

    InterruptibleSleeper sleeper_;
};

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_NBDWATCHCONTEXT_H_
