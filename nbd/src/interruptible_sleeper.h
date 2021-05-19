

#ifndef SRC_COMMON_INTERRUPTIBLE_SLEEPER_H_
#define SRC_COMMON_INTERRUPTIBLE_SLEEPER_H_

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace cyprestore {
namespace nbd {
/**
 * InterruptibleSleeper 实现可 interruptible 的 sleep 功能.
 * 正常情况下 wait_for 超时, 接收到退出信号之后, 程序会立即被唤醒,
 * 退出 while 循环, 并执行 cleanup 代码.
 */
class InterruptibleSleeper {
public:
    /**
     * @brief wait_for 等待指定时间，如果接受到退出信号立刻返回
     *
     * @param[in] time 指定wait时长
     *
     * @return false-收到退出信号 true-超时后退出
     */
    template <typename R, typename P>
    bool WaitFor(std::chrono::duration<R, P> const &time) {
        std::unique_lock<std::mutex> lock(mtx_);
        return !cv_.wait_for(lock, time, [&] { return terminate_; });
    }

    /**
     * @brief interrupt 给当前wait发送退出信号
     */
    void Interrupt() {
        std::unique_lock<std::mutex> lock(mtx_);
        terminate_ = true;
        cv_.notify_all();
    }

private:
    std::condition_variable cv_;
    std::mutex mtx_;
    bool terminate_ = false;
};

}  // namespace nbd
}  // namespace cyprestore

#endif  // SRC_COMMON_INTERRUPTIBLE_SLEEPER_H_
