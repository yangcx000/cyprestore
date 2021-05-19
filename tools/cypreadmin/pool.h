/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_TOOLS_CYPREADMIN_POOL_H_
#define CYPRESTORE_TOOLS_CYPREADMIN_POOL_H_

#include <brpc/channel.h>

#include <string>

#include "common/pb/types.pb.h"
#include "extentmanager/pb/cluster.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class Pool {
public:
    Pool(const Options &options, brpc::Channel *channel) : options_(options) {
        stub_ = new extentmanager::pb::ClusterService_Stub(channel);
    }

    ~Pool() {
        delete stub_;
    }

    int Run();
    int Create();
    int Query();
    int List();
    int Update();
    int Delete();
    int Init();
    void Describe(const common::pb::Pool &pool);

private:
    common::pb::PoolType getPoolType();
    std::string getPoolType(common::pb::PoolType type);
    common::pb::FailureDomain getFailureDomain();
    std::string getFailureDomain(common::pb::FailureDomain domain);

    Options options_;
    std::string pool_id_;
    extentmanager::pb::ClusterService_Stub *stub_;
};

}  // namespace tools
}  // namespace cyprestore

#endif  // CYPRESTORE_TOOLS_CYPREADMIN_POOL_H_
