#ifndef SPIN_LOCK_H_
#define SPIN_LOCK_H_

#include <atomic>

// spin lock 1.
class SpinLock1 {
public:
    // SpinLock() : lock_(false) {}
    virtual void lock() {
        for (;;) {
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                break;
            }
            while (lock_.load(std::memory_order_relaxed)) {
                __builtin_ia32_pause();
            }
        }
    }
    virtual void unlock() {
        lock_.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> lock_ = { false };
};

class spin_lock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    spin_lock() = default;
    spin_lock(const spin_lock &) = delete;
    spin_lock &operator=(const spin_lock &) = delete;

    void lock() {
        while (flag.test_and_set(std::memory_order_acquire))
            __builtin_ia32_pause();
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }
};

#endif