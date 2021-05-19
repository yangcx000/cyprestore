/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "heartbeat.h"

#include <iostream>
#include <string>

namespace cyprestore {
namespace tools {

int Heartbeat::Report() {
    brpc::Controller cntl;
    extentmanager::pb::ReportHeartbeatRequest req;
    extentmanager::pb::ReportHeartbeatResponse resp;

    common::pb::Endpoint endpoint;
    endpoint.set_public_ip("127.0.0." + std::to_string(options_.es_id));
    endpoint.set_public_port(8080);
    endpoint.set_private_ip("127.0.0." + std::to_string(options_.es_id));
    endpoint.set_private_port(8081);

    common::pb::ExtentServer server;
    server.set_id(options_.es_id);
    server.set_name("es-" + std::to_string(options_.es_id));
    server.set_size(0);
    server.set_capacity(4398046511104);
    server.set_pool_id(options_.pool_id);
    server.set_host(options_.es_host);
    server.set_rack(options_.es_rack);
    *server.mutable_endpoint() = endpoint;
    *req.mutable_es() = server;

    stub_->ReportHeartbeat(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to report heartbeat, err: "
                  << resp.status().message() << std::endl;
        return -1;
    }

    std::cout << "Reportheart succeed, es_id: " << options_.es_id << std::endl;
    return 0;
}

int Heartbeat::Run() {
    int ret = 0;

    switch (GetCommand(options_.cmd)) {
        case Command::kReport:
            ret = Report();
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
