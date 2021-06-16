/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_COMMON_CONFIG_H_
#define CYPRESTORE_COMMON_CONFIG_H_

#include <butil/macros.h>

#include <ostream>
#include <sstream>
#include <string>

namespace cyprestore {
namespace common {

// Sections
const std::string kSectionCommon = "common";
const std::string kSectionNames = "names";
const std::string kSectionNetwork = "network";
const std::string kSectionLog = "log";
const std::string kSectionBrpc = "brpc";
const std::string kSectionSpdk = "spdk";
const std::string kSectionRocksDB = "rocksdb";
const std::string kSectionEtcd = "etcd";

const std::string kSectionAccess = "access";
const std::string kSectionSetManager = "setmanager";
const std::string kSectionExtentManager = "extentmanager";
const std::string kSectionExtentServer = "extentserver";

const int kMinimumPort = 8000;
const int kMaximumPort = 10000;

// common
struct CommonCfg {
    bool valid() const {
        return instance != -1 && !subsys.empty() && !module.empty()
               && !region.empty() && !myname.empty();
    }

    int instance;
    int set_id;
    std::string subsys;
    std::string module;
    std::string region;
    std::string myname;
    std::string set_prefix;
};

// names
struct NamesCfg {
    std::string access;
    std::string setmanager;
    std::string extentmanager;
    std::string extentserver;
};

// network
struct NetworkCfg {
    bool valid() const {
        return !public_ip.empty() && !private_ip.empty()
               && (public_port >= kMinimumPort && public_port <= kMaximumPort)
               && (private_port >= kMinimumPort
                   && private_port <= kMaximumPort);
    }

    std::string public_endpoint() const {
        std::stringstream ss(
                public_ip, std::ios_base::app | std::ios_base::out);
        ss << ":" << public_port;
        return ss.str();
    }

    std::string public_ip;
    std::string private_ip;
    int public_port;
    int private_port;
};

// log
struct LogCfg {
    bool valid() {
        return maxsize > 0 && !logpath.empty() && !prefix.empty()
               && !suffix.empty();
    }

    uint64_t maxsize;
    std::string level;
    std::string logpath;
    std::string prefix;
    std::string suffix;
};

// brpc
struct BrpcCfg {
    int num_threads;
    int max_concurrency;
    std::string brpc_worker_core_mask;
};

// rocksdb
struct RocksDBCfg {
    std::string db_path;
    int compact_threads;
};

// etcd
struct EtcdCfg {
    bool valid() {
        return !initial_cluster.empty();
    }

    std::string initial_cluster;
};

// spdk
struct SpdkCfg {
    int shm_id;
    int mem_channel;
    int mem_size;
    int master_core;
    size_t num_pci_addr;
    bool no_pci;
    bool hugepage_single_segments;
    bool unlink_hugepage;
    std::string core_mask;
    std::string huge_dir;
    std::string name;
    std::string json_config_file;
    std::string rpc_addr;
};

// access
struct AccessCfg {
    int update_interval_sec;
};

// setmanager
struct SetManagerCfg {
    int obsolete_interval_sec;
    int num_sets;
    std::string set_ip;
    int set_port;
};

// ExtentManager
struct ExtentManagerCfg {
    int heartbeat_interval_sec;
    uint64_t extent_size;
    uint32_t max_block_size;
    int rg_per_es;
    int router_inst_shift;
    int gc_interval_sec;
    int gc_begin;
    int gc_end;
    std::string em_ip;
    int em_port;
};

// ExtentServer
struct ExtentServerCfg {
    int heartbeat_interval_sec;
    std::string pool_id;
    std::string rack;
    std::string dev_name;
    std::string dev_type;
    std::string replication_type;
    int spdk_request_ring_size;
    int num_spdk_workers;
    std::string spdk_worker_core_mask;
    int slow_request_time;
};

// Config
class Config {
public:
    Config() {}

    int Init(const std::string &module, const std::string &config_path);

    const CommonCfg &common() const {
        return common_;
    }

    const NamesCfg &names() const {
        return names_;
    }

    const NetworkCfg &network() const {
        return network_;
    }

    const LogCfg &log() const {
        return log_;
    }

    const BrpcCfg brpc() const {
        return brpc_;
    }

    const RocksDBCfg rocksdb() const {
        return rocksdb_;
    }

    const EtcdCfg &etcd() const {
        return etcd_;
    }

    const SpdkCfg &spdk() const {
        return spdk_;
    }

    const AccessCfg &access() const {
        return access_;
    }

    const SetManagerCfg setmanager() const {
        return setmanager_;
    }

    const ExtentManagerCfg &extentmanager() const {
        return extentmanager_;
    }

    const ExtentServerCfg &extentserver() const {
        return extentserver_;
    }

    bool DebugLog() const {
        return log_.level == "DEBUG";
    }

private:
    DISALLOW_COPY_AND_ASSIGN(Config);
    int Parse(const std::string &config_path);
    friend std::ostream &operator<<(std::ostream &os, const Config &config);

    CommonCfg common_;
    NamesCfg names_;
    NetworkCfg network_;
    LogCfg log_;
    BrpcCfg brpc_;
    RocksDBCfg rocksdb_;
    EtcdCfg etcd_;
    SpdkCfg spdk_;
    AccessCfg access_;
    SetManagerCfg setmanager_;
    ExtentManagerCfg extentmanager_;
    ExtentServerCfg extentserver_;
};

std::ostream &operator<<(std::ostream &os, const Config &config);

}  // namespace common

common::Config &GlobalConfig();

}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_CONFIG_H_
