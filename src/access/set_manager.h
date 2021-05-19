/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */
#ifndef CYPRESTORE_ACCESS_SET_MANAGER_H
#define CYPRESTORE_ACCESS_SET_MANAGER_H

#include <brpc/channel.h>

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "common/pb/types.pb.h"

namespace cyprestore {
namespace access {

using common::pb::Status;

class SetMgr {
public:
    SetMgr() = default;
    ~SetMgr() = default;

    int Init();
    static void UpdateSetsFunc(void *arg);
    Status QuerySet(const std::string &user_id, common::pb::SetRoute *setroute);
    Status QuerySetById(int set_id, common::pb::SetRoute *setroute);
    Status AllocateSet(
            const std::string user_name, common::pb::PoolType type,
            common::pb::SetRoute *setroute);

private:
    void UpdateSets();
    // set_id -> setroute
    std::map<int, std::shared_ptr<common::pb::SetRoute>> set_map_;
    boost::shared_mutex set_mutex_;
    std::shared_ptr<brpc::Channel> set_channel_;
};

}  // namespace access
}  // namespace cyprestore

#endif  // CYPRESTORE_ACCESS_SET_MANAGER_H
