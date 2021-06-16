/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTMANAGER_EXTENT_SERVER_H_
#define CYPRESTORE_EXTENTMANAGER_EXTENT_SERVER_H_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cereal/access.hpp>
#include <cereal/types/string.hpp>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/constants.h"
#include "common/pb/types.pb.h"
#include "common/status.h"
#include "enum.h"
#include "kvstore/rocks_store.h"
#include "utils/chrono.h"
#include "utils/serializer.h"

namespace cyprestore {
namespace extentmanager {

class ExtentServer {
public:
    ExtentServer() = default;
    ExtentServer(
            int id, const std::string &name, ESStatus status,
            const std::string &public_ip, int public_port,
            const std::string &private_ip, int private_port, uint64_t size,
            uint64_t capacity, const std::string &pool_id,
            const std::string &host, const std::string &rack)
            : id_(id), name_(name), status_(kESStatusOk), public_ip_(public_ip),
              public_port_(public_port), private_ip_(private_ip),
              private_port_(private_port), used_(size), capacity_(capacity),
              pool_id_(pool_id), host_(host), rack_(rack) {
        create_time_ = utils::Chrono::DateString();
        update_time_ = create_time_;
        rgs_ = 0;
        primary_rgs_ = 0;
        secondary_rgs_ = 0;
    }
    ~ExtentServer() = default;

    std::string kv_key() {
        std::stringstream ss(
                common::kESKvPrefix, std::ios_base::app | std::ios_base::out);
        ss << "_" << pool_id_ << "_" << id_;
        return ss.str();
    }

    int get_id() {
        return id_;
    }
    uint64_t get_capacity() {
        return capacity_;
    }
    uint64_t get_used() {
        return used_;
    }
    std::string get_host() {
        return host_;
    }
    std::string get_rack() {
        return rack_;
    }
    void incr_rgs() {
        rgs_++;
    }
    int get_rgs() {
        return rgs_;
    }
    void incr_pri_rgs() {
        primary_rgs_++;
    }
    int get_pri_rgs() {
        return primary_rgs_;
    }
    void incr_sec_rgs() {
        secondary_rgs_++;
    }
    int get_sec_rgs() {
        return secondary_rgs_;
    }
    void set_weight(uint64_t weight) {
        weight_ = weight;
    }
    uint64_t get_weight() {
        return weight_;
    }
    void decr_weight() {
        weight_--;
    }
    common::pb::ExtentServer ToPbExtentServer();
    common::pb::Endpoint ToPbEndpoint();
    common::pb::EsInstance ToPbEsInstance();

private:
    friend class EsManager;
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive &archive, const std::uint32_t version) {
        archive(id_);
        archive(name_);
        archive(status_);
        archive(public_ip_);
        archive(public_port_);
        archive(private_ip_);
        archive(private_port_);
        archive(used_);
        archive(capacity_);
        archive(pool_id_);
        archive(host_);
        archive(rack_);
        archive(create_time_);
        archive(update_time_);
    }

    int id_;
    // need?
    std::string name_;
    ESStatus status_;
    std::string public_ip_;
    int public_port_;
    std::string private_ip_;
    int private_port_;
    uint64_t used_;
    uint64_t capacity_;
    std::string pool_id_;
    std::string host_;
    std::string rack_;
    std::string create_time_;
    std::string update_time_;

    // TODO need to persist?
    int rgs_;
    int primary_rgs_;
    int secondary_rgs_;
    // weight_ = capacity / extent_size
    uint64_t weight_;
};

using common::Status;

class EsManager {
public:
    EsManager(std::shared_ptr<kvstore::RocksStore> kv_store)
            : kv_store_(kv_store) {}
    ~EsManager() {}

    Status create_es(
            int id, const std::string &name, const std::string &public_ip,
            int public_port, const std::string &private_ip, int private_port,
            uint64_t size, uint64_t capacity, const std::string &pool_id,
            const std::string &host, const std::string &rack);
    Status query_es(int es_id, common::pb::ExtentServer *es);
    Status delete_es(int es_id);
    Status delete_all_es();
    Status update_es(
            int es_id, const std::string &name, const std::string &public_ip,
            int public_port, const std::string &private_ip, int private_port,
            uint64_t size);
    Status list_es(std::vector<common::pb::ExtentServer> *ess);
    Status list_es(std::vector<std::shared_ptr<ExtentServer>> *ess);
    Status query_es_router(
            const std::vector<int> &es_group, common::pb::ExtentRouter *router);
    bool es_exist(int es_id);
    uint32_t es_count() {
        return es_map_.size();
    }
    uint64_t total_capacity();
    uint64_t total_used();
    bool recovery_from_store(const std::string &pool_id);

private:
    Status check_old_es(
            int id, const std::string &name, const std::string &public_ip,
            int public_port, const std::string &private_ip, int private_port);
    Status check_new_es(
            const std::string &name, const std::string &public_ip,
            int public_port, const std::string &private_ip, int private_port);

    std::map<int, std::shared_ptr<ExtentServer>> es_map_;
    boost::shared_mutex mutex_;
    std::shared_ptr<kvstore::RocksStore> kv_store_;
};

}  // namespace extentmanager
}  // namespace cyprestore

CEREAL_CLASS_VERSION(cyprestore::extentmanager::ExtentServer, 0);

#endif  // CYPRESTORE_EXTENTMANAGER_EXTENT_SERVER_H_
