/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "scrub.h"

#include <iostream>
#include <set>

namespace cyprestore {
namespace tools {

int Scrub::Verify() {
    // query blob
    if (queryBlob() != 0) {
        return -1;
    }

    uint32_t count = blob_size_ / extent_size_;
    for (uint32_t i = 1; i <= count; ++i) {
        // query router
        std::string extent_id = options_.blob_id + "." + std::to_string(i);
        common::pb::ExtentRouter router;
        if (queryRouter(extent_id, &router) != 0) {
            return -1;
        }
        if (doVerify(extent_id, router) != 0) {
            return -1;
        }
        std::cout << "Scrub extent: " << extent_id << " success." << std::endl;
    }
    return 0;
}

int Scrub::getConns(
        const common::pb::ExtentRouter &router,
        std::vector<common::ConnectionPtr> *conns) {
    auto conn = conn_pool_->GetConnection(
            router.primary().public_ip(), router.primary().public_port());
    if (conn == nullptr) {
        std::cerr << "Couldn't connect to peer es" << std::endl;
        return -1;
    }
    conns->push_back(conn);
    for (int i = 0; i < router.secondaries().size(); ++i) {
        auto conn = conn_pool_->GetConnection(
                router.secondaries(i).public_ip(),
                router.secondaries(i).public_port());
        if (conn == nullptr) {
            std::cerr << "Couldn't connect to peer es" << std::endl;
            return -1;
        }
        conns->push_back(conn);
    }
    return 0;
}

int Scrub::doVerify(
        const std::string &extent_id, const common::pb::ExtentRouter &router) {
    int replicas = 1 + router.secondaries().size();
    std::vector<common::ConnectionPtr> conns;
    if (getConns(router, &conns) != 0) {
        return -1;
    }
    for (uint64_t offset = 0; offset < extent_size_;
         offset = offset + block_size_) {
        std::set<uint32_t> crc;
        std::vector<IOContext> ctxs(replicas);
        for (int i = 0; i < replicas; ++i) {
            extentserver::pb::ExtentIOService_Stub stub(
                    conns[i]->channel.get());
            extentserver::pb::ScrubRequest req;
            req.set_extent_id(extent_id);
            req.set_offset(offset);
            req.set_size(block_size_);
            stub.Scrub(
                    &ctxs[i].cntl, &req, &ctxs[i].response, brpc::DoNothing());
        }

        for (int i = 0; i < replicas; ++i) {
            brpc::Join(ctxs[i].cntl.call_id());
            if (ctxs[i].cntl.Failed()) {
                std::cerr << "Couldn't send scrub request: "
                          << ctxs[i].cntl.ErrorText() << std::endl;
                return -1;
            } else if (ctxs[i].response.status().code() != 0) {
                std::cerr << "Couldn't scrub: "
                          << ctxs[i].response.status().message() << std::endl;
                return -1;
            }
            crc.insert(ctxs[i].response.crc32());
        }
        if (crc.size() != 1) {
            return -1;
        }
    }
    return 0;
}

int Scrub::queryRouter(
        const std::string &extent_id, common::pb::ExtentRouter *router) {
    brpc::Controller cntl;
    extentmanager::pb::QueryRouterRequest request;
    extentmanager::pb::QueryRouterResponse response;
    request.set_extent_id(extent_id);
    router_stub_->QueryRouter(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send query blob request failed, err: " << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to query blob, err: "
                  << response.status().message() << std::endl;
        return -1;
    }
    *router = response.router();
    return 0;
}

int Scrub::queryBlob() {
    brpc::Controller cntl;
    extentmanager::pb::QueryBlobRequest request;
    extentmanager::pb::QueryBlobResponse response;
    request.set_blob_id(options_.blob_id);
    stub_->QueryBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send query blob request failed, err: " << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to query blob, err: "
                  << response.status().message() << std::endl;
        return -1;
    }
    blob_size_ = response.blob().size();
    extent_size_ = response.extent_size();
    return 0;
}

int Scrub::init() {
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

    stub_ = new extentmanager::pb::ResourceService_Stub(&channel_);
    if (!stub_) {
        std::cerr << "Failed to new manager service stub" << std::endl;
        return -1;
    }

    router_stub_ = new extentmanager::pb::RouterService_Stub(&channel_);
    if (!router_stub_) {
        std::cerr << "Failed to new manager service stub" << std::endl;
        return -1;
    }

    conn_pool_.reset(new common::ConnectionPool());

    initialized_ = true;
    return 0;
}

int Scrub::Exec() {
    if (!initialized_ && init() != 0) {
        return -1;
    }

    switch (options_.cmd) {
        case Cmd::kVerify:
            return Verify();
            break;
        default:
            break;
    }
    return -1;
}

}  // namespace tools
}  // namespace cyprestore
