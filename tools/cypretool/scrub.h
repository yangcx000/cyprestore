/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#ifndef CYPRESTORE_TOOLS_CYPRETOOL_SCRUB_H
#define CYPRESTORE_TOOLS_CYPRETOOL_SCRUB_H

#include <brpc/channel.h>
#include <brpc/server.h>

#include <memory>
#include <string>
#include <vector>

#include "common/connection_pool.h"
#include "common/pb/types.pb.h"
#include "extentmanager/pb/resource.pb.h"
#include "extentmanager/pb/router.pb.h"
#include "extentserver/pb/extent_io.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class Scrub {
public:
    Scrub(const Options &options)
            : options_(options), initialized_(false), stub_(nullptr) {}

    int Exec();
    int Verify();

private:
    int init();
    int getConns(
            const common::pb::ExtentRouter &router,
            std::vector<common::ConnectionPtr> *conns);
    int doVerify(
            const std::string &extent_id,
            const common::pb::ExtentRouter &router);
    int
    queryRouter(const std::string &extent_id, common::pb::ExtentRouter *router);
    int queryBlob();

    const uint64_t block_size_ = 4 << 10;  // 4k
    uint64_t blob_size_;
    uint64_t extent_size_;
    Options options_;
    bool initialized_;
    brpc::Channel channel_;
    extentmanager::pb::ResourceService_Stub *stub_;
    extentmanager::pb::RouterService_Stub *router_stub_;
    std::unique_ptr<common::ConnectionPool> conn_pool_;

    struct IOContext {
        extentserver::pb::ScrubResponse response;
        brpc::Controller cntl;
    };
};

}  // namespace tools
}  // namespace cyprestore

#endif  // CYPRESTORE_TOOLS_CYPRETOOL_SCRUB_H
