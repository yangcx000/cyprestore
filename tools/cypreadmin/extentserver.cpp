/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "extentserver.h"

#include <iostream>

namespace cyprestore {
namespace tools {

void ExtentServer::Describe(const common::pb::ExtentServer &es) {
    std::cout << "***********************ExtentServer "
                 "Information**************************"
              << "\n\t es_id:" << es.id() << "\n\t es_name:" << es.name()
              << "\n\t public_ip:" << es.endpoint().public_ip()
              << "\n\t public_port:" << es.endpoint().public_port()
              << "\n\t private_ip:" << es.endpoint().private_ip()
              << "\n\t private_port:" << es.endpoint().private_port()
              << "\n\t size:" << es.size() << "\n\t capacity:" << es.capacity()
              << "\n\t pool_id:" << es.pool_id() << "\n\t host:" << es.host()
              << "\n\t rack:" << es.rack() << "\n\t status:" << es.status()
              << "\n\t create_date:" << es.create_date()
              << "\n\t update_date:" << es.update_date() << std::endl;
}

int ExtentServer::Query() {
    brpc::Controller cntl;
    extentmanager::pb::QueryExtentServerRequest req;
    extentmanager::pb::QueryExtentServerResponse resp;

    req.set_es_id(options_.es_id);
    req.set_pool_id(options_.pool_id);

    stub_->QueryExtentServer(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to query es, err: " << resp.status().message()
                  << std::endl;
        return -1;
    }

    if (resp.has_es()) {
        Describe(resp.es());
    } else {
        std::cout << "ExtentServer " << options_.es_id << " not exists"
                  << std::endl;
    }

    return 0;
}

int ExtentServer::List() {
    brpc::Controller cntl;
    extentmanager::pb::ListExtentServersRequest req;
    extentmanager::pb::ListExtentServersResponse resp;

    req.set_pool_id(options_.pool_id);
    stub_->ListExtentServers(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to list extent servers, err: "
                  << resp.status().message() << std::endl;
        return -1;
    }

    std::cout << "There are " << resp.ess_size()
              << " extentservers:" << std::endl;
    for (int i = 0; i < resp.ess_size(); i++) {
        Describe(resp.ess(i));
    }

    return 0;
}

int ExtentServer::Delete() {
    brpc::Controller cntl;
    extentmanager::pb::DeleteExtentServerRequest req;
    extentmanager::pb::DeleteExtentServerResponse resp;

    req.set_es_id(options_.es_id);
    req.set_pool_id(options_.pool_id);

    stub_->DeleteExtentServer(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to query es, err: " << resp.status().message()
                  << std::endl;
        return -1;
    }

    std::cout << "Delete es: " << options_.es_id << " succeed." << std::endl;
    return 0;
}

int ExtentServer::Run() {
    int ret = 0;

    switch (GetCommand(options_.cmd)) {
        case Command::kQuery:
            ret = Query();
            break;
        case Command::kList:
            ret = List();
            break;
        case Command::kDelete:
            ret = Delete();
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
