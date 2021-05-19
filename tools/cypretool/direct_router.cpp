/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "direct_router.h"

namespace cyprestore {
namespace tools {

void DirectRouter::Describe(const common::pb::ExtentRouter &er) {
    std::cout << "*********************Router Information*********************"
              << "\nrg id:" << er.rg_id() << std::endl;
    std::cout << "\t primary es id:" << er.primary().es_id()
              << "\t primary es public ip:" << er.primary().public_ip()
              << "\t primary es public port:" << er.primary().public_port()
              << std::endl;
    for (int i = 0; i < er.secondaries_size(); i++) {
        std::cout << "\t secondary es id:" << er.secondaries(i).es_id()
                  << "\t secondary es public ip:"
                  << er.secondaries(i).public_ip()
                  << "\t secondary es public port:"
                  << er.secondaries(i).public_port() << std::endl;
    }
}

int DirectRouter::Query() {
    brpc::Controller cntl;
    extentmanager::pb::QueryRouterRequest request;
    extentmanager::pb::QueryRouterResponse response;

    request.set_extent_id(options_.extent_id);

    stub_->QueryRouter(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to query router, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    Describe(response.router());
    return 0;
}

int DirectRouter::init() {
    brpc::ChannelOptions channel_options;
    channel_options.protocol = options_.protocal;
    channel_options.connection_type = options_.connection_type;
    channel_options.timeout_ms = options_.timeout_ms;
    channel_options.max_retry = options_.max_retry;

    if (channel_.Init(options_.ExtentManagerAddr().c_str(), &channel_options)
        != 0) {
        std::cerr << "Failed to initialize channel" << std::endl;
        return -1;
    }

    stub_ = new extentmanager::pb::RouterService_Stub(&channel_);
    if (!stub_) {
        std::cerr << "Failed to new access service stub" << std::endl;
        return -1;
    }
    initialized_ = true;
    return 0;
}

int DirectRouter::Exec() {
    if (!initialized_ && init() != 0) {
        return -1;
    }
    switch (options_.cmd) {
        case Cmd::kQuery:
            return Query();
        default:
            return -1;
    }
}

}  // namespace tools
}  // namespace cyprestore
