/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "utils/timer_thread.h"

#include <butil/logging.h>
#include <butil/time.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>

namespace cyprestore {
namespace utils {

int Timer::init() {
    fd_ = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd_ < 0) {
        LOG(ERROR) << "Failed to create timerfd " << PrintSysError();
        return -1;
    }

    struct itimerspec ts;
    struct timespec zero = { 0, 0 };
    /* Initial expiration */
    clock_gettime(CLOCK_REALTIME, &ts.it_value);
    /* Timer interval */
    ts.it_interval =
            butil::milliseconds_from(zero, options_.interval_milliseconds);

    int ret = timerfd_settime(fd_, TFD_TIMER_ABSTIME, &ts, NULL);
    if (ret != 0) {
        LOG(ERROR) << "Failed to set timerfd time " << PrintSysError();
        return -1;
    }

    return 0;
}

int TimerThread::start() {
    if (started_) return 0;

    epfd_ = epoll_create(kMaxEvents_);
    if (epfd_ < 0) {
        LOG(FATAL) << "Failed to create epoll " << PrintSysError();
        return -1;
    }

    int ret = pthread_create(&tid_, NULL, TimerThread::pthread_callback, this);
    if (ret != 0) {
        LOG(FATAL) << "Failed to create timer pthread " << PrintSysError();
        return -1;
    }

    started_ = true;
    return 0;
}

int TimerThread::stop() {
    if (pthread_cancel(tid_) != 0) {
        LOG(ERROR) << "Failed to cancel timer pthread " << PrintSysError();
        return -1;
    }

    if (pthread_join(tid_, NULL) != 0) {
        LOG(ERROR) << "Failed to join timer pthread " << PrintSysError();
        return -1;
    }

    return 0;
}

int TimerThread::add_timer(const TimerPtr &timer) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = timer.get();

    int ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, timer->fd_, &ev);
    if (ret < 0) {
        LOG(ERROR) << "Failed to add timer to epoll, " << PrintSysError()
                   << ", epfd_:" << epfd_ << ", timer_fd:" << timer->fd_;
        return -1;
    }

    return 0;
}

int TimerThread::create_timer(const TimerOptions &timer_options) {
    TimerPtr timer = std::make_shared<Timer>(timer_options);
    if (!timer) {
        LOG(ERROR) << "Failed to allocate timer";
        return -1;
    }

    if (timer->init() != 0) {
        return -1;
    }

    if (timers_.find(timer->fd_) != timers_.end()) {
        LOG(ERROR) << "Timer alread exists, timer_fd:" << timer->fd_;
        return -1;
    }

    if (add_timer(timer) != 0) {
        return -1;
    }

    timers_[timer->fd_] = timer;
    return 0;
}

int TimerThread::remove_timer(TimerId timer_id) {
    auto it = timers_.find(timer_id);
    if (it != timers_.end()) {
        it->second->cancel();
    }

    return 0;
}

void TimerThread::delete_timer(int fd) {
    epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
    timers_.erase(fd);
}

void TimerThread::run() {
    struct epoll_event events[kMaxEvents_];
    uint64_t data = 0;
    int fds = 0;

    while (true) {
        fds = epoll_wait(epfd_, events, kMaxEvents_, -1);
        if (fds < 0) {
            LOG(ERROR) << "Failed to wait epoll " << PrintSysError();
            continue;
        }

        for (int i = 0; i < fds; ++i) {
            Timer *timer = static_cast<Timer *>(events[i].data.ptr);
            if (read(timer->fd_, &data, sizeof(uint64_t)) != sizeof(uint64_t)) {
                LOG(ERROR) << "Failed to read timer event " << PrintSysError();
                continue;
            }

            // timer cancelled or run once
            if (timer->canceled()) {
                delete_timer(timer->fd_);
                LOG(INFO) << "Timer has removed, fd:" << timer->fd_;
                continue;
            }

            timer->run();
            if (!timer->repeated()) {
                delete_timer(timer->fd_);
                LOG(INFO) << "Non-repeated timer, remove it after run, fd:"
                          << timer->fd_;
            }
        }
    }
}

void *TimerThread::pthread_callback(void *arg) {
    static_cast<TimerThread *>(arg)->run();
    return nullptr;
}

static pthread_once_t g_timer_thread_once = PTHREAD_ONCE_INIT;
static TimerThread *g_timer_thread = nullptr;

static void init_global_timer_thread() {
    g_timer_thread = new (std::nothrow) TimerThread();
    if (g_timer_thread == nullptr) {
        LOG(FATAL) << "Fail to new g_timer_thread";
        return;
    }

    int rc = g_timer_thread->start();
    if (rc != 0) {
        LOG(FATAL) << "Fail to start timer_thread";
        delete g_timer_thread;
        g_timer_thread = nullptr;
        return;
    }
}

TimerThread *GetOrCreateGlobalTimerThread() {
    pthread_once(&g_timer_thread_once, init_global_timer_thread);
    return g_timer_thread;
}

TimerThread *GetGlobalTimerThread() {
    return g_timer_thread;
}

}  // namespace utils
}  // namespace cyprestore
