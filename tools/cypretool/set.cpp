/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "set.h"

#include <iostream>

namespace cyprestore {
namespace tools {

void Set::Describe(const common::pb::SetRoute &setroute) {
    std::cout << "*********************Set Information*********************"
              << "\n\t id:" << setroute.set_id()
              << "\n\t name:" << setroute.set_name()
              << "\n\t extentmanager ip:" << setroute.ip()
              << "\n\t extentmanager port:" << setroute.port() << std::endl;
}

int Set::Query() {
    brpc::Controller cntl;
    access::pb::QuerySetRequest request;
    access::pb::QuerySetResponse response;

    request.set_user_id(options_.user_id);
    stub_->QuerySet(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to list user, err: " << response.status().message()
                  << std::endl;
        return -1;
    }
    Describe(response.setroute());
    return 0;
}

// int Pool::List() {
//    brpc::Controller cntl;
//    extentmanager::pb::ListPoolsRequest req;
//    extentmanager::pb::ListPoolsResponse resp;
//
//    stub_->ListPools(&cntl, &req, &resp, NULL);
//    if (!cntl.Failed()) {
//        if (resp.status().code() != 0) {
//            std::cerr << "Failed to create pool"
//                       << ", pool_id:" << pool_id_
//                       << ", err: " << resp.status().message();
//            return -1;
//        }
//    } else {
//        std::cerr << "Send request failed, err:" << cntl.ErrorText();
//        return -1;
//    }
//
//    std::cout << "List pools succeed, pool_size:" << resp.pools_size();
//    for (int i = 0; i < resp.pools_size(); i++) {
//        Describe(resp.pools(i));
//    }
//
//    return 0;
//}

int Set::init() {
    brpc::ChannelOptions channel_options;
    channel_options.protocol = options_.protocal;
    channel_options.connection_type = options_.connection_type;
    channel_options.timeout_ms = options_.timeout_ms;
    channel_options.max_retry = options_.max_retry;

    if (channel_.Init(options_.AccessAddr().c_str(), &channel_options) != 0) {
        std::cerr << "Failed to initialize channel" << std::endl;
        return -1;
    }

    stub_ = new access::pb::AccessService_Stub(&channel_);
    if (!stub_) {
        std::cerr << "Failed to new access service stub" << std::endl;
        return -1;
    }

    initialized_ = true;
    return 0;
}

int Set::Exec() {
    if (!initialized_ && init() != 0) {
        return -1;
    }
    int ret = -1;
    switch (options_.cmd) {
        case Cmd::kQuery:
            ret = Query();
            break;
        // case Cmd::kList:
        //    ret = List();
        //    break;
        default:
            ret = -1;
            std::cerr << "Unknown command, cmd:" << options_.cmd << std::endl;
            break;
    }

    return ret;
}

}  // namespace tools
}  // namespace cyprestore
