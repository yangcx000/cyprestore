#include "common/rwlock.h"

#include <pthread.h>

#include "gtest/gtest.h"

namespace cyprestore {
namespace common {
namespace {

struct SharedObject {
    SharedObject() : count(0) {}
    uint64_t Get() {
        ReadLock rlock(lock);
        return count;
    }

    void Add() {
        WriteLock wlock(lock);
        ++count;
    }

    uint64_t count;
    RWLock lock;
};

void *read_func(void *arg) {
    SharedObject *obj = static_cast<SharedObject *>(arg);
    uint32_t loop_counts = 100000;
    while (loop_counts > 0) {
        obj->Get();
        --loop_counts;
    }

    return nullptr;
}

void *write_func(void *arg) {
    SharedObject *obj = static_cast<SharedObject *>(arg);
    uint32_t loop_counts = 100000;
    while (loop_counts > 0) {
        obj->Add();
        --loop_counts;
    }
    return nullptr;
}

TEST(RWLockTest, TestReadLock) {
    const int kNumThreads = 5;
    std::vector<pthread_t> tids(kNumThreads);
    SharedObject obj;
    for (size_t i = 0; i < tids.size(); ++i) {
        int ret = pthread_create(&tids[i], nullptr, read_func, &obj);
        ASSERT_EQ(ret, 0);
    }

    for (size_t i = 0; i < tids.size(); ++i) {
        pthread_join(tids[i], nullptr);
    }
}

TEST(RWLockTest, TestWriteLock) {
    const int kNumThreads = 5;
    std::vector<pthread_t> tids(kNumThreads);
    SharedObject obj;
    for (size_t i = 0; i < tids.size(); ++i) {
        int ret = pthread_create(&tids[i], nullptr, write_func, &obj);
        ASSERT_EQ(ret, 0);
    }

    for (size_t i = 0; i < tids.size(); ++i) {
        pthread_join(tids[i], nullptr);
    }
}

TEST(RWLockTest, TestReadWriteLock) {
    const int kNumThreads = 5;
    std::vector<pthread_t> tids(kNumThreads);
    SharedObject obj;
    for (size_t i = 0; i < tids.size(); ++i) {
        int ret = 0;
        if (i % 2 == 0) {
            ret = pthread_create(&tids[i], nullptr, write_func, &obj);
        } else {
            ret = pthread_create(&tids[i], nullptr, read_func, &obj);
        }
        ASSERT_EQ(ret, 0);
    }

    for (size_t i = 0; i < tids.size(); ++i) {
        pthread_join(tids[i], nullptr);
    }
}

}  // namespace
}  // namespace common
}  // namespace cyprestore
