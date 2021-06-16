/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "extent_server.h"

#include "butil/logging.h"
#include "common/error_code.h"
#include "utils/chrono.h"
#include "utils/pb_transfer.h"

namespace cyprestore {
namespace extentmanager {

common::pb::ExtentServer ExtentServer::ToPbExtentServer() {
    common::pb::ExtentServer pb_es;

    pb_es.set_id(id_);
    pb_es.set_name(name_);
    common::pb::Endpoint ep = ToPbEndpoint();
    *pb_es.mutable_endpoint() = ep;
    pb_es.set_size(used_);
    pb_es.set_capacity(capacity_);
    pb_es.set_pool_id(pool_id_);
    pb_es.set_host(host_);
    pb_es.set_rack(rack_);
    pb_es.set_status(utils::ToPbExtentServerStatus(status_));
    pb_es.set_create_date(create_time_);
    pb_es.set_update_date(update_time_);

    return pb_es;
}

common::pb::Endpoint ExtentServer::ToPbEndpoint() {
    common::pb::Endpoint pb_ep;

    pb_ep.set_public_ip(public_ip_);
    pb_ep.set_public_port(public_port_);
    pb_ep.set_private_ip(private_ip_);
    pb_ep.set_private_port(private_port_);

    return pb_ep;
}

common::pb::EsInstance ExtentServer::ToPbEsInstance() {
    common::pb::EsInstance pb_ei;

    pb_ei.set_es_id(id_);
    pb_ei.set_public_ip(public_ip_);
    pb_ei.set_public_port(public_port_);
    pb_ei.set_private_ip(private_ip_);
    pb_ei.set_private_port(private_port_);

    return pb_ei;
}

Status EsManager::create_es(
        int id, const std::string &name, const std::string &public_ip,
        int public_port, const std::string &private_ip, int private_port,
        uint64_t size, uint64_t capacity, const std::string &pool_id,
        const std::string &host, const std::string &rack) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    if (es_map_.find(id) != es_map_.end()) {
        return Status(common::CYPRE_EM_ES_DUPLICATED, "Es already exist");
        ;
    }

    // check whether es information conflict
    auto status = check_new_es(
            name, public_ip, public_port, private_ip, private_port);
    if (!status.ok()) {
        return status;
    }

    auto es = std::make_shared<ExtentServer>(
            id, name, kESStatusOk, public_ip, public_port, private_ip,
            private_port, size, capacity, pool_id, host, rack);

    std::string value = utils::Serializer<ExtentServer>::Encode(*es.get());
    auto kv_status = kv_store_->Put(es->kv_key(), value);
    if (!kv_status.ok()) {
        LOG(ERROR) << "Persist es meta to store failed, es_id: " << id;
        return Status(
                common::CYPRE_EM_STORE_ERROR,
                "persist es meta to store failed");
    }
    es_map_[es->id_] = es;
    return Status();
}

Status EsManager::check_new_es(
        const std::string &name, const std::string &public_ip, int public_port,
        const std::string &private_ip, int private_port) {
    bool conflict = false;
    std::string message;
    for (auto &entry : es_map_) {
        if (entry.second->name_ == name) {
            LOG(ERROR) << "ES information conflict, please check name: "
                       << name;
            message = "es name conflict";
            conflict = true;
            break;
        }
        if (entry.second->public_ip_ == public_ip
            && entry.second->public_port_ == public_port) {
            LOG(ERROR) << "ES information conflict, please check public addr: "
                       << public_ip << ":" << public_port;
            message = "es public addr conflict";
            conflict = true;
            break;
        }
        if (entry.second->private_ip_ == private_ip
            && entry.second->private_port_ == private_port) {
            LOG(ERROR) << "ES information conflict, please check private addr: "
                       << private_ip << ":" << private_port;
            message = "es private addr conflict";
            conflict = true;
            break;
        }
    }
    if (conflict) {
        return Status(common::CYPRE_ER_INVALID_ARGUMENT, message);
    }
    return Status();
}

Status EsManager::check_old_es(
        int es_id, const std::string &name, const std::string &public_ip,
        int public_port, const std::string &private_ip, int private_port) {
    bool changed = false;
    std::string message;
    auto iter = es_map_.find(es_id);
    if (iter->second->name_ != name) {
        LOG(ERROR) << "ES information changed, please check name: " << name;
        message = "es name changed";
        changed = true;
    }
    if (iter->second->public_ip_ != public_ip
        || iter->second->public_port_ != public_port) {
        LOG(ERROR) << "ES information changed, please check public addr: "
                   << public_ip << ":" << public_port;
        message = "es public addr changed";
        changed = true;
    }
    if (iter->second->private_ip_ != private_ip
        || iter->second->private_port_ != private_port) {
        LOG(ERROR) << "ES information changed, please check private addr: "
                   << private_ip << ":" << private_port;
        message = "es private addr changed";
        changed = true;
    }
    if (changed) {
        return Status(common::CYPRE_ER_INVALID_ARGUMENT, message);
    }
    return Status();
}

Status EsManager::update_es(
        int es_id, const std::string &name, const std::string &public_ip,
        int public_port, const std::string &private_ip, int private_port,
        uint64_t size) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = es_map_.find(es_id);
    if (iter == es_map_.end()) {
        return Status(common::CYPRE_EM_ES_NOT_FOUND, "not found extentserver");
    }

    auto status = check_old_es(
            es_id, name, public_ip, public_port, private_ip, private_port);
    if (!status.ok()) {
        return status;
    }
    iter->second->used_ = size;
    std::string value =
            utils::Serializer<ExtentServer>::Encode(*(iter->second.get()));
    auto kv_status = kv_store_->Put(iter->second->kv_key(), value);
    if (!kv_status.ok()) {
        LOG(ERROR) << "Persist es meta to store failed, es_id: " << es_id;
        return Status(
                common::CYPRE_EM_STORE_ERROR,
                "persist es meta to store failed");
    }
    return Status();
}

Status EsManager::delete_es(int es_id) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    auto iter = es_map_.find(es_id);
    if (iter == es_map_.end()) {
        return Status(common::CYPRE_EM_ES_NOT_FOUND, "not found extentserver");
    }
    auto kv_status = kv_store_->Delete(iter->second->kv_key());
    if (!kv_status.ok()) {
        LOG(ERROR) << "Delete es meta from store failed, es_id: " << es_id;
        return Status(
                common::CYPRE_EM_DELETE_ERROR,
                "delete es meta from store failed");
    }
    es_map_.erase(iter);
    return Status();
}

Status EsManager::delete_all_es() {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    for (auto &m : es_map_) {
        auto kv_status = kv_store_->Delete(m.second->kv_key());
        if (!kv_status.ok()) {
            LOG(ERROR) << "Delete es meta from store failed, es_id: "
                       << m.first;
        }
    }
    es_map_.clear();
    return Status();
}

Status EsManager::query_es(int es_id, common::pb::ExtentServer *es) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = es_map_.find(es_id);
    if (iter == es_map_.end()) {
        return Status(common::CYPRE_EM_ES_NOT_FOUND, "not found extentserver");
    }
    *es = iter->second->ToPbExtentServer();
    return Status();
}

Status EsManager::query_es_router(
        const std::vector<int> &es_group, common::pb::ExtentRouter *router) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (size_t i = 0; i < es_group.size(); i++) {
        auto iter = es_map_.find(es_group[i]);
        if (i == 0) {
            common::pb::EsInstance *pri = router->mutable_primary();
            *pri = iter->second->ToPbEsInstance();
        } else {
            common::pb::EsInstance *sec = router->add_secondaries();
            *sec = iter->second->ToPbEsInstance();
        }
    }
    return Status();
}

bool EsManager::es_exist(int es_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    return es_map_.find(es_id) != es_map_.end();
}

uint64_t EsManager::total_capacity() {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    uint64_t all_ca = 0;
    for (auto &e : es_map_) {
        all_ca += e.second->capacity_;
    }
    return all_ca;
}

uint64_t EsManager::total_used() {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    uint64_t all_used = 0;
    for (auto &e : es_map_) {
        all_used += e.second->used_;
    }
    return all_used;
}

Status EsManager::list_es(std::vector<common::pb::ExtentServer> *ess) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &e : es_map_) {
        ess->push_back(e.second->ToPbExtentServer());
    }
    return Status();
}

Status EsManager::list_es(std::vector<std::shared_ptr<ExtentServer>> *ess) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &e : es_map_) {
        ess->push_back(e.second);
    }
    return Status();
}

// only called for process first start
bool EsManager::recovery_from_store(const std::string &pool_id) {
    // no need to lock
    std::unique_ptr<kvstore::KVIterator> iter;
    std::stringstream ss(
            common::kESKvPrefix, std::ios_base::app | std::ios_base::out);
    ss << "_" << pool_id;
    std::string prefix = ss.str();
    auto status = kv_store_->ScanPrefix(prefix, &iter);
    if (!status.ok()) {
        LOG(ERROR) << "Scan extent server meta from store failed, status: "
                   << status.ToString();
        return false;
    }
    std::vector<std::string> tmp;
    while (iter->Valid()) {
        tmp.push_back(iter->value());
        iter->Next();
    }
    for (auto &v : tmp) {
        auto es = std::make_shared<ExtentServer>();
        if (utils::Serializer<ExtentServer>::Decode(v, *es.get())) {
            es_map_[es->id_] = es;
        } else {
            return false;
        }
    }
    LOG(INFO) << "Recovery extent server meta from store success, include "
              << es_map_.size() << " extent servers.";
    return true;
}

}  // namespace extentmanager
}  // namespace cyprestore
