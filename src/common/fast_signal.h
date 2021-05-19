/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 *
 */

#ifndef CYPRESTORE_COMMON_FAST_SIGNAL_H_
#define CYPRESTORE_COMMON_FAST_SIGNAL_H_

#include <pthread.h>
#include <semaphore.h>

#include <atomic>

#include "common/builtin.h"

namespace cyprestore {
namespace common {

class FastSignal {
public:
    FastSignal() : event_(false) {
        sem_init(&sem_, 0, 0);
    }
    ~FastSignal() {
        sem_destroy(&sem_);
    }

    int Signal() {
        bool expect = false;
        if (event_.compare_exchange_strong(expect, true)) {
            return sem_post(&sem_);
        }
        return 0;
    }
    void Broadcast(int N) {
        for (int i = 0; i < N; i++) {
            sem_post(&sem_);
        }
    }
    int Wait() {
        bool expect = true;
        if (event_.compare_exchange_strong(expect, false)) {
            return 0;
        }
        return sem_wait(&sem_);
    }

private:
    FastSignal(const FastSignal &) = delete;
    void operator=(const FastSignal &) = delete;

    std::atomic<bool> event_;
    sem_t sem_;
};

class SpinedSignal {
public:
    SpinedSignal() : event_(false) {}
    ~SpinedSignal() {}

    void Signal() {
        event_.store(true, std::memory_order_release);
    }
    void Broadcast(int N) {
        return;
    }
    void Wait(volatile bool *stopFlag, int nSpin = 1024) {
        while (true) {
            bool expect = true;
            if (event_.compare_exchange_weak(
                        expect, false, std::memory_order_acquire)) {
                break;
            }
            for (int n = 1; n < nSpin; n <<= 1) {
                for (int i = 0; i < n; i++) {
                    cypres_cpu_pause();
                }
                if (event_.compare_exchange_weak(
                            expect, false, std::memory_order_acquire)) {
                    return;
                }
            }
            sched_yield();
            if (stopFlag && *stopFlag == true) {
                break;
            }
        }
    }

private:
    SpinedSignal(const SpinedSignal &s) = delete;
    void operator=(const SpinedSignal &s) = delete;

    std::atomic<bool> event_;
};

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_FAST_SIGNAL_H_
