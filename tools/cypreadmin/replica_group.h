/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_TOOLS_CYPREADMIN_REPLICA_GROUP_H_
#define CYPRESTORE_TOOLS_CYPREADMIN_REPLICA_GROUP_H_

#include <brpc/channel.h>

#include <string>

#include "common/pb/types.pb.h"
#include "extentmanager/pb/cluster.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class ReplicaGroup {
public:
    ReplicaGroup(const Options &options, brpc::Channel *channel)
            : options_(options) {
        stub_ = new extentmanager::pb::ClusterService_Stub(channel);
    }

    ~ReplicaGroup() {
        delete stub_;
    }

    int Run();
    int Query();
    int List();
    void Describe(const common::pb::ReplicaGroup &rg);

private:
    Options options_;
    extentmanager::pb::ClusterService_Stub *stub_;
};

}  // namespace tools
}  // namespace cyprestore

#endif  // CYPRESTORE_TOOLS_CYPREADMIN_REPLICA_GROUP_H_
