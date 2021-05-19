/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#pragma once

#include <brpc/channel.h>

#include <chrono>
#include <vector>

#include "extentmanager/pb/resource.pb.h"
#include "extentmanager/pb/router.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class RouterBench {
public:
    RouterBench(const Options &options)
            : options_(options), initialized_(false), router_stub_(nullptr),
              blob_stub_(nullptr) {}

    int Exec();
    int BenchQuery();
    int BenchCreate();

private:
    int init();
    void Query(std::vector<std::string> blob_ids);
    int CreateBlob(int num);
    int PreCreateBlob();
    std::time_t getTimeStamp();
    Options options_;
    bool initialized_;
    // for router query
    brpc::Channel router_channel_;
    extentmanager::pb::RouterService_Stub *router_stub_;

    // for blob create
    brpc::Channel blob_channel_;
    extentmanager::pb::ResourceService_Stub *blob_stub_;
    std::vector<std::string> blob_ids_;
};

}  // namespace tools
}  // namespace cyprestore
