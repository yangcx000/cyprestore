/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "pool.h"

#include <iostream>

namespace cyprestore {
namespace tools {

static const std::string HDD = "hdd";
static const std::string SSD = "ssd";
static const std::string NVMESSD = "nvmessd";

static const std::string NODE = "node";
static const std::string HOST = "host";
static const std::string RACK = "rack";

common::pb::PoolType Pool::getPoolType() {
    if (options_.pool_type == HDD) {
        return common::pb::PoolType::POOL_TYPE_HDD;
    } else if (options_.pool_type == SSD) {
        return common::pb::PoolType::POOL_TYPE_SSD;
    } else if (options_.pool_type == NVMESSD) {
        return common::pb::PoolType::POOL_TYPE_NVME_SSD;
    }

    return common::pb::PoolType::POOL_TYPE_UNKNOWN;
}

std::string Pool::getPoolType(common::pb::PoolType type) {
    if (type == common::pb::PoolType::POOL_TYPE_HDD) {
        return HDD;
    } else if (type == common::pb::PoolType::POOL_TYPE_SSD) {
        return SSD;
    } else if (type == common::pb::PoolType::POOL_TYPE_NVME_SSD) {
        return NVMESSD;
    }
    return "unknown";
}

common::pb::FailureDomain Pool::getFailureDomain() {
    if (options_.failure_domain == NODE) {
        return common::pb::FailureDomain::FAILURE_DOMAIN_NODE;
    } else if (options_.failure_domain == HOST) {
        return common::pb::FailureDomain::FAILURE_DOMAIN_HOST;
    } else if (options_.failure_domain == RACK) {
        return common::pb::FailureDomain::FAILURE_DOMAIN_RACK;
    }

    return common::pb::FailureDomain::FAILURE_DOMAIN_UNKNOWN;
}

std::string Pool::getFailureDomain(common::pb::FailureDomain domain) {
    if (domain == common::pb::FailureDomain::FAILURE_DOMAIN_NODE) {
        return NODE;
    } else if (domain == common::pb::FailureDomain::FAILURE_DOMAIN_HOST) {
        return HOST;
    } else if (domain == common::pb::FailureDomain::FAILURE_DOMAIN_RACK) {
        return RACK;
    }
    return "unknown";
}

void Pool::Describe(const common::pb::Pool &pool) {
    std::cout << "*********************Pool Information*********************"
              << "\n\t id:" << pool.id() << "\n\t name:" << pool.name()
              << "\n\t type:" << getPoolType(pool.type())
              << "\n\t status:" << pool.status()
              << "\n\t replica_count:" << pool.replica_count()
              << "\n\t rg_count:" << pool.rg_count()
              << "\n\t es_count:" << pool.es_count() << "\n\t failure_domain:"
              << getFailureDomain(pool.failure_domain())
              << "\n\t create_date:" << pool.create_date()
              << "\n\t update_date:" << pool.update_date()
              << "\n\t capacity:" << pool.capacity()
              << "\n\t size:" << pool.size()
              << "\n\t extent_size:" << pool.extent_size()
              << "\n\t num_extents:" << pool.num_extents() << std::endl;
}

int Pool::Create() {
    brpc::Controller cntl;
    extentmanager::pb::CreatePoolRequest req;
    extentmanager::pb::CreatePoolResponse resp;

    req.set_name(options_.pool_name);
    req.set_type(getPoolType());
    req.set_replica_count(options_.replica_count);
    req.set_es_count(options_.es_count);
    req.set_failure_domain(getFailureDomain());

    stub_->CreatePool(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to create pool, err: " << resp.status().message()
                  << std::endl;
        return -1;
    }

    std::cout << "Create pool " << options_.pool_name << " succeed"
              << ", pool_id:" << resp.pool_id() << std::endl;
    return 0;
}

int Pool::Query() {
    brpc::Controller cntl;
    extentmanager::pb::QueryPoolRequest req;
    extentmanager::pb::QueryPoolResponse resp;

    req.set_pool_id(pool_id_);
    stub_->QueryPool(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to query pool"
                  << ", pool_id:" << pool_id_
                  << ", err: " << resp.status().message() << std::endl;
        return -1;
    }

    if (resp.has_pool()) {
        Describe(resp.pool());
    } else {
        std::cout << "Pool " << pool_id_ << " not exists" << std::endl;
    }

    return 0;
}

int Pool::List() {
    brpc::Controller cntl;
    extentmanager::pb::ListPoolsRequest req;
    extentmanager::pb::ListPoolsResponse resp;

    stub_->ListPools(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to create pool"
                  << ", pool_id:" << pool_id_
                  << ", err: " << resp.status().message() << std::endl;
        return -1;
    }

    std::cout << "There are " << resp.pools_size() << " pools:" << std::endl;
    for (int i = 0; i < resp.pools_size(); i++) {
        Describe(resp.pools(i));
    }

    return 0;
}

int Pool::Update() {
    brpc::Controller cntl;
    extentmanager::pb::UpdatePoolRequest req;
    extentmanager::pb::UpdatePoolResponse resp;

    req.set_pool_id(pool_id_);
    req.set_name(options_.pool_new_name);
    req.set_type(getPoolType());
    req.set_replica_count(options_.replica_count);
    req.set_es_count(options_.es_count);
    req.set_failure_domain(getFailureDomain());
    stub_->UpdatePool(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to rename pool"
                  << ", pool_id:" << pool_id_
                  << ", err: " << resp.status().message() << std::endl;
        return -1;
    }

    std::cout << "Update pool " << pool_id_ << " succeed" << std::endl;
    return 0;
}

int Pool::Delete() {
    brpc::Controller cntl;
    extentmanager::pb::DeletePoolRequest req;
    extentmanager::pb::DeletePoolResponse resp;

    req.set_pool_id(pool_id_);
    stub_->DeletePool(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to rename pool"
                  << ", pool_id:" << pool_id_
                  << ", err: " << resp.status().message() << std::endl;
        return -1;
    }

    std::cout << "Delete pool " << pool_id_ << " succeed" << std::endl;
    return 0;
}

int Pool::Init() {
    brpc::Controller cntl;
    extentmanager::pb::InitPoolRequest req;
    extentmanager::pb::InitPoolResponse resp;

    req.set_pool_id(pool_id_);
    stub_->InitPool(&cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (resp.status().code() != 0) {
        std::cerr << "Failed to init pool"
                  << ", pool_id:" << pool_id_
                  << ", err: " << resp.status().message() << std::endl;
        return -1;
    }

    std::cout << "Init pool " << pool_id_ << " succeed" << std::endl;
    return 0;
}

int Pool::Run() {
    int ret = 0;

    pool_id_ = options_.pool_id;

    switch (GetCommand(options_.cmd)) {
        case Command::kCreate:
            ret = Create();
            break;
        case Command::kQuery:
            ret = Query();
            break;
        case Command::kList:
            ret = List();
            break;
        case Command::kRename:
            ret = Update();
            break;
        case Command::kInit:
            ret = Init();
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
