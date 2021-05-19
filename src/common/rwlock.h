/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_COMMON_RWLOCK_H_
#define CYPRESTORE_COMMON_RWLOCK_H_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <mutex>
#include <string>
#include <unordered_map>

namespace cyprestore {
namespace common {

typedef boost::shared_mutex RWLock;
typedef boost::unique_lock<RWLock> WriteLock;
typedef boost::shared_lock<RWLock> ReadLock;

class ExtentLockMgr {
public:
    ExtentLockMgr() = default;
    ~ExtentLockMgr() = default;

    std::mutex &GetLock(const std::string &extent_id) {
        {
            ReadLock lock(lock_);
            auto it = extent_lock_map_.find(extent_id);
            if (it != extent_lock_map_.end()) return it->second;
        }

        {
            WriteLock lock(lock_);
            auto it = extent_lock_map_.find(extent_id);
            if (it != extent_lock_map_.end()) return it->second;

            return extent_lock_map_[extent_id];
        }
    }

private:
    std::unordered_map<std::string, std::mutex> extent_lock_map_;
    RWLock lock_;
};

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_RWLOCK_H_
