/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#ifndef CYPRESTORE_TOOLS_DIRECT_ROUTER_H_
#define CYPRESTORE_TOOLS_DIRECT_ROUTER_H_

#include <brpc/channel.h>

#include "common/pb/types.pb.h"
#include "extentmanager/pb/router.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class DirectRouter {
public:
    DirectRouter(const Options &options)
            : options_(options), initialized_(false), stub_(nullptr) {}

    int Exec();
    int Query();

private:
    void Describe(const common::pb::ExtentRouter &router);
    int init();
    Options options_;
    bool initialized_;
    brpc::Channel channel_;
    extentmanager::pb::RouterService_Stub *stub_;
};

}  // namespace tools
}  // namespace cyprestore

#endif  // CYPRESTORE_TOOLS_DIRECT_ROUTER_H_
