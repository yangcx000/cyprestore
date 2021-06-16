/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_SETMANAGER_SETMANAGER_H_
#define CYPRESTORE_SETMANAGER_SETMANAGER_H_

#include <brpc/server.h>
#include <butil/macros.h>

#include <memory>
#include <vector>

#include "common/config.h"
#include "common/pb/types.pb.h"
#include "utils/chrono.h"

namespace cyprestore {
namespace setmanager {

void SignHandler(int sig);

struct Set;

typedef std::shared_ptr<Set> SetPtr;
typedef std::unordered_map<int, SetPtr> SetMap;
typedef std::vector<Set> SetArray;

struct Set {
    Set() {}
    Set(const common::pb::Set &pb_set_) : pb_set(pb_set_) {
        struct timespec tm = utils::Chrono::CurrentTime();
        created_time = tm.tv_sec;
    }

    bool obsoleted() const {
        struct timespec tm = utils::Chrono::CurrentTime();
        if (tm.tv_sec
            > (created_time
               + GlobalConfig().setmanager().obsolete_interval_sec)) {
            std::cout << "Obsoleted set\n";
            return true;
        }
        return false;
    }

    bool empty() {
        return pb_set.pools_size() == 0;
    }

    time_t created_time;
    common::pb::Set pb_set;
};

class SetManager {
public:
    SetManager() = default;
    ~SetManager() = default;

    static SetManager &GlobalInstance();

    int Init(const std::string &config_path);
    int RegisterServices();
    void Start();
    void Destroy();
    void WaitToQuit();

    void AddSet(const common::pb::Set &pb_set);
    Set AllocateSet(common::pb::PoolType pool_type);
    SetArray ListSets();
    size_t num_sets() {
        return set_map_.size();
    }

    bool Available() {
        return GlobalConfig().setmanager().num_sets
               == static_cast<int>(num_sets());
    }

private:
    DISALLOW_COPY_AND_ASSIGN(SetManager);

    brpc::Server server_;
    SetMap set_map_;
    std::mutex mutex_;
    volatile bool stop_;
};

}  // namespace setmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_SETMANAGER_SETMANAGER_H_
