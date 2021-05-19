/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "replica_group.h"

#include <iostream>

namespace cyprestore {
namespace tools {

void ReplicaGroup::Describe(const common::pb::ReplicaGroup &rg) {
    std::cout << "*********************Replication "
                 "Information*********************"
              << "\n\t rg_id:" << rg.id() << "\n\t rg_status:" << rg.status()
              << "\n\t nr_extents:" << rg.nr_extents()
              << "\n\t pool_id:" << rg.pool_id()
              << "\n\t create_date:" << rg.create_date()
              << "\n\t update_date:" << rg.update_date()
              << "\n\t extent_size:" << rg.extent_size() << std::endl;

    for (int i = 0; i < rg.extent_servers_size(); i++) {
        std::cout << "\t index: " << i << ", es_id:" << rg.extent_servers(i)
                  << " " << std::endl;
    }
}

int ReplicaGroup::Query() {
    brpc::Controller cntl;
    extentmanager::pb::QueryReplicationGroupRequest req;
    extentmanager::pb::QueryReplicationGroupResponse resp;

    req.set_rg_id(options_.rg_id);
    req.set_pool_id(options_.pool_id);

    stub_->QueryReplicationGroup(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to query rg, err: " << resp.status().message()
                  << std::endl;
        return -1;
    }

    if (resp.has_rg()) {
        Describe(resp.rg());
    } else {
        std::cout << "ReplicaGroup " << options_.rg_id << " not exists"
                  << std::endl;
    }

    return 0;
}

int ReplicaGroup::List() {
    brpc::Controller cntl;
    extentmanager::pb::ListReplicationGroupsRequest req;
    extentmanager::pb::ListReplicationGroupsResponse resp;

    req.set_pool_id(options_.pool_id);
    stub_->ListReplicationGroups(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to list replication groups, err: "
                  << resp.status().message() << std::endl;
        return -1;
    }

    std::cout << "There are " << resp.rgs_size()
              << " replication groups:" << std::endl;
    for (int i = 0; i < resp.rgs_size(); i++) {
        Describe(resp.rgs(i));
    }
    return 0;
}

int ReplicaGroup::Run() {
    int ret = 0;

    switch (GetCommand(options_.cmd)) {
        case Command::kQuery:
            ret = Query();
            break;
        case Command::kList:
            ret = List();
            break;
        default:
            ret = -1;
            std::cerr << "Unknown command, cmd:" << options_.cmd << std::endl;
            break;
    }
    return ret;
}

}  // namespace tools
}  // namespace cyprestore
