/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_UTILS_TIMER_THREAD_H_
#define CYPRESTORE_UTILS_TIMER_THREAD_H_

#include <butil/macros.h>
#include <pthread.h>
#include <sys/epoll.h>

#include <cstring>
#include <memory>
#include <unordered_map>

// XXX: move to common utils
#define PrintSysError() std::strerror(errno)

namespace cyprestore {
namespace utils {

typedef void (*TimerFunc)(void *);

struct TimerOptions {
    TimerOptions()
            : repeated(false), interval_milliseconds(0), func(nullptr),
              arg(nullptr) {}

    TimerOptions(
            bool repeated_, int64_t interval_milliseconds_, TimerFunc func_,
            void *arg_)
            : repeated(repeated_),
              interval_milliseconds(interval_milliseconds_), func(func_),
              arg(arg_) {}

    bool repeated;
    int64_t interval_milliseconds;
    TimerFunc func;
    void *arg;
};

class Timer {
public:
    Timer(const TimerOptions &options)
            : options_(options), canceled_(false), fd_(-1) {}

    ~Timer() {
        fd_ != -1 && close(fd_);
    }

    int init();

    void run() {
        options_.func(options_.arg);
    }

    void cancel() {
        canceled_ = true;
    }
    bool canceled() {
        return canceled_;
    }
    bool repeated() {
        return options_.repeated;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(Timer);
    friend class TimerThread;

    TimerOptions options_;
    bool canceled_;
    int fd_;
};

class TimerThread {
public:
    typedef int TimerId;
    typedef std::shared_ptr<Timer> TimerPtr;
    typedef std::unordered_map<TimerId, TimerPtr> TimerMap;

    TimerThread() : started_(false), epfd_(-1) {}

    ~TimerThread() {
        epfd_ != -1 && close(epfd_);
    }

    int start();
    int create_timer(const TimerOptions &timer_options);
    int remove_timer(TimerId timer_id);

    size_t timer_sizes() const {
        return timers_.size();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(TimerThread);
    // maybe coredump
    int stop();
    int add_timer(const TimerPtr &timer);
    void delete_timer(int fd);
    void run();
    static void *pthread_callback(void *arg);

    const int kMaxEvents_ = 1024;
    bool started_;
    int epfd_;
    pthread_t tid_;
    TimerMap timers_;
};

TimerThread *GetOrCreateGlobalTimerThread();
TimerThread *GetGlobalTimerThread();

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_TIMER_THREAD_H_
