/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_TOOLS_CYPREADMIN_EXTENTSERVER_H_
#define CYPRESTORE_TOOLS_CYPREADMIN_EXTENTSERVER_H_

#include <brpc/channel.h>

#include <string>

#include "common/pb/types.pb.h"
#include "extentmanager/pb/cluster.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class ExtentServer {
public:
    ExtentServer(const Options &options, brpc::Channel *channel)
            : options_(options) {
        stub_ = new extentmanager::pb::ClusterService_Stub(channel);
    }

    ~ExtentServer() {
        delete stub_;
    }

    int Run();
    int Query();
    int List();
    int Delete();
    void Describe(const common::pb::ExtentServer &es);

private:
    Options options_;
    extentmanager::pb::ClusterService_Stub *stub_;
};

}  // namespace tools
}  // namespace cyprestore

#endif  // CYPRESTORE_TOOLS_CYPREADMIN_EXTENTSERVER_H_
