/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "config.h"

#include <cstdlib>
#include <iomanip>  // setfill/setw
#include <iostream>

#include "utils/ini_parser.h"

namespace cyprestore {
namespace common {

int Config::Init(const std::string &module, const std::string &config_path) {
    if (Parse(config_path) != 0) {
        return -1;
    }

    if (module != common_.module) {
        std::cerr << "Config module doesn't match, expect:" << module
                  << ", actual:" << common_.module << std::endl;
        return -1;
    }

    return 0;
}

int Config::Parse(const std::string &config_path) {
    utils::INIParser ini_parser(config_path);

    if (ini_parser.Parse() != 0) {
        std::cerr << "Couldn't parse config file at " << config_path
                  << std::endl;
        return -1;
    }

    if (!ini_parser.HasSection(kSectionCommon)) {
        std::cerr << "Couldn't find [common] section, at least have it"
                  << std::endl;
        return -1;
    }

    // Common section
    common_.instance = static_cast<int>(
            ini_parser.GetInteger(kSectionCommon, "instance", -1));
    common_.set_id = static_cast<int>(
            ini_parser.GetInteger(kSectionCommon, "set_id", -1));
    common_.subsys = ini_parser.GetString(kSectionCommon, "subsys", "");
    common_.module = ini_parser.GetString(kSectionCommon, "module", "");
    common_.region = ini_parser.GetString(kSectionCommon, "region", "");
    common_.myname = ini_parser.GetString(kSectionCommon, "myname", "");
    common_.set_prefix = ini_parser.GetString(kSectionCommon, "set_prefix", "");

    if (!common_.valid()) {
        std::cerr << "Invalid [common] section, please check it" << std::endl;
        return -1;
    }

    // Names
    if (ini_parser.HasSection(kSectionNames)) {
        names_.access = ini_parser.GetString(kSectionNames, "access", "");
        names_.setmanager =
                ini_parser.GetString(kSectionNames, "setmanager", "");
        names_.extentmanager =
                ini_parser.GetString(kSectionNames, "extentmanager", "");
        names_.extentserver =
                ini_parser.GetString(kSectionNames, "extentserver", "");
    }

    // Network
    if (ini_parser.HasSection(kSectionNetwork)) {
        network_.public_ip =
                ini_parser.GetString(kSectionNetwork, "public_ip", "");
        network_.public_port = static_cast<int>(
                ini_parser.GetInteger(kSectionNetwork, "public_port", 0));
        network_.private_ip = ini_parser.HasValue(kSectionNetwork, "private_ip")
                                      ? ini_parser.GetString(
                                              kSectionNetwork, "private_ip", "")
                                      : network_.public_ip;
        network_.private_port =
                ini_parser.HasValue(kSectionNetwork, "private_port")
                        ? static_cast<int>(ini_parser.GetInteger(
                                kSectionNetwork, "private_port", 0))
                        : network_.public_port;
    }

    if (!network_.valid()) {
        std::cerr << "Invalid [network] section, please check it" << std::endl;
        return -1;
    }

    // Log
    if (ini_parser.HasSection(kSectionLog)) {
        log_.level = ini_parser.GetString(kSectionLog, "level", "INFO");
        log_.logpath = ini_parser.GetString(kSectionLog, "logpath", "");
        log_.prefix = ini_parser.GetString(kSectionLog, "prefix", "");
        log_.suffix = ini_parser.GetString(kSectionLog, "suffix", "");

        std::string strMaxSize =
                ini_parser.GetString(kSectionLog, "maxsize", "");
        if (strMaxSize.empty()) {
            std::cerr << "Couldn't find 'maxsize' in [log] section, please "
                         "check it"
                      << std::endl;
            return -1;
        }

        char *p;
        long value = strtol(strMaxSize.c_str(), &p, 10);
        log_.maxsize = static_cast<uint64_t>(
                value * ((*p) == 'G' ? 1 << 30 : 1 << 20));
    }

    if (!log_.valid()) {
        std::cerr << "Invalid [log] section, please check it" << std::endl;
        return -1;
    }

    // brpc
    brpc_.num_threads = static_cast<int>(
            ini_parser.GetInteger(kSectionBrpc, "num_threads", 8));
    brpc_.max_concurrency = static_cast<int>(
            ini_parser.GetInteger(kSectionBrpc, "max_concurrency", 0));
    brpc_.brpc_worker_core_mask = ini_parser.GetString(
            kSectionBrpc, "brpc_worker_core_mask", "");

    // rocksdb
    if (ini_parser.HasSection(kSectionRocksDB)) {
        rocksdb_.db_path = ini_parser.GetString(kSectionRocksDB, "db_path", "");
        rocksdb_.compact_threads =
                ini_parser.GetInteger(kSectionRocksDB, "compact_threads", 2);
    }

    // etcd
    etcd_.initial_cluster =
            ini_parser.GetString(kSectionEtcd, "initial_cluster", "");

    // spdk
    if (ini_parser.HasSection(kSectionSpdk)) {
        spdk_.shm_id = static_cast<int>(
                ini_parser.GetInteger(kSectionSpdk, "shm_id", -1));
        spdk_.mem_channel = static_cast<int>(
                ini_parser.GetInteger(kSectionSpdk, "mem_channel", -1));
        spdk_.mem_size = static_cast<int>(
                ini_parser.GetInteger(kSectionSpdk, "mem_size", -1));
        spdk_.master_core = static_cast<int>(
                ini_parser.GetInteger(kSectionSpdk, "master_core", -1));
        spdk_.num_pci_addr = static_cast<int>(
                ini_parser.GetInteger(kSectionSpdk, "num_pci_addr", 0));
        spdk_.no_pci = ini_parser.GetBoolean(kSectionSpdk, "no_pci", false);
        spdk_.hugepage_single_segments = ini_parser.GetBoolean(
                kSectionSpdk, "hugepage_single_segments", false);
        spdk_.unlink_hugepage =
                ini_parser.GetBoolean(kSectionSpdk, "unlink_hugepage", false);
        spdk_.core_mask = ini_parser.GetString(kSectionSpdk, "core_mask", "");
        spdk_.huge_dir = ini_parser.GetString(kSectionSpdk, "huge_dir", "");
        spdk_.name = ini_parser.GetString(kSectionSpdk, "name", "");
        spdk_.json_config_file =
                ini_parser.GetString(kSectionSpdk, "json_config_file", "");
        spdk_.rpc_addr = ini_parser.GetString(kSectionSpdk, "rpc_addr", "");
    }

    // Access
    if (ini_parser.HasSection(kSectionAccess)) {
        access_.update_interval_sec = static_cast<int>(ini_parser.GetInteger(
                kSectionAccess, "update_interavl_sec", 5));
    }

    // SetManager
    if (ini_parser.HasSection(kSectionSetManager)) {
        setmanager_.obsolete_interval_sec =
                static_cast<int>(ini_parser.GetInteger(
                        kSectionSetManager, "obsolete_interval_sec", 5));
        setmanager_.num_sets = static_cast<int>(
                ini_parser.GetInteger(kSectionSetManager, "num_sets", 0));
        setmanager_.set_ip =
                ini_parser.GetString(kSectionSetManager, "set_ip", "");
        setmanager_.set_port = static_cast<int>(
                ini_parser.GetInteger(kSectionSetManager, "set_port", 0));
    }

    // ExtentManager
    if (ini_parser.HasSection(kSectionExtentManager)) {
        // TODO:
        extentmanager_.heartbeat_interval_sec =
                static_cast<int>(ini_parser.GetInteger(
                        kSectionExtentManager, "heartbeat_interval_sec", 5));
        extentmanager_.extent_size =
                static_cast<uint64_t>(ini_parser.GetInteger(
                        kSectionExtentManager, "extent_size", 1073741824));
        extentmanager_.max_block_size =
                static_cast<uint32_t>(ini_parser.GetInteger(
                        kSectionExtentManager, "max_block_size", 67108864));
        extentmanager_.rg_per_es = static_cast<int>(
                ini_parser.GetInteger(kSectionExtentManager, "rg_per_es", 50));
        extentmanager_.router_inst_shift =
                static_cast<int>(ini_parser.GetInteger(
                        kSectionExtentManager, "router_inst_shift", 6));
        extentmanager_.gc_interval_sec = static_cast<int>(ini_parser.GetInteger(
                kSectionExtentManager, "gc_interval_sec", 60));
        extentmanager_.gc_begin = static_cast<int>(
                ini_parser.GetInteger(kSectionExtentManager, "gc_begin", 2));
        extentmanager_.gc_end = static_cast<int>(
                ini_parser.GetInteger(kSectionExtentManager, "gc_end", 5));
        extentmanager_.em_ip =
                ini_parser.GetString(kSectionExtentManager, "em_ip", "");
        extentmanager_.em_port = static_cast<int>(
                ini_parser.GetInteger(kSectionExtentManager, "em_port", 0));
    }

    // ExtentServer
    if (ini_parser.HasSection(kSectionExtentServer)) {
        // TODO:
        extentserver_.heartbeat_interval_sec =
                static_cast<int>(ini_parser.GetInteger(
                        kSectionExtentServer, "heartbeat_interval_sec", 5));
        extentserver_.pool_id =
                ini_parser.GetString(kSectionExtentServer, "pool_id", "");
        extentserver_.rack =
                ini_parser.GetString(kSectionExtentServer, "rack", "");
        extentserver_.dev_name =
                ini_parser.GetString(kSectionExtentServer, "dev_name", "");
        extentserver_.dev_type =
                ini_parser.GetString(kSectionExtentServer, "dev_type", "");
        if (extentserver_.dev_name.empty() || extentserver_.dev_type.empty()) {
            std::cerr << "dev_name and dev_type must be set,"
                      << ", dev_name:" << extentserver_.dev_name
                      << ", dev_type:" << extentserver_.dev_type << std::endl;
            return -1;
        }
        extentserver_.replication_type = ini_parser.GetString(
                kSectionExtentServer, "replication_type", "standard");
        extentserver_.spdk_request_ring_size =
                static_cast<int>(ini_parser.GetInteger(
                        kSectionExtentServer, "spdk_request_ring_size",
                        52428800));
        extentserver_.num_spdk_workers = static_cast<int>(ini_parser.GetInteger(
                kSectionExtentServer, "num_spdk_workers", 2));
        extentserver_.spdk_worker_core_mask = ini_parser.GetString(kSectionExtentServer, "spdk_worker_core_mask", "");
        extentserver_.slow_request_time = static_cast<int>(ini_parser.GetInteger(
                kSectionExtentServer, "slow_request_time", 400));
    }

    return 0;
}

std::ostream &operator<<(std::ostream &os, const Config &config) {
    os << std::setfill('-') << std::setw(40) << '-' << "[Common]"
       << std::setfill('-') << std::setw(40) << '-'
       << "\nSubsys:" << config.common_.subsys
       << "\nModule:" << config.common_.module
       << "\nRegion:" << config.common_.region
       << "\nSetId:" << config.common_.set_id
       << "\nSetPrefix:" << config.common_.set_prefix
       << "\nMyname:" << config.common_.myname
       << "\nInstance:" << config.common_.instance << std::endl
       << std::setfill('-') << std::setw(40) << '-' << "[Names]"
       << std::setfill('-') << std::setw(40) << '-'
       << "\nAccess:" << config.names_.access
       << "\nSetManager:" << config.names_.setmanager
       << "\nExtentManager:" << config.names_.extentmanager
       << "\nExtentServer:" << config.names_.extentserver << std::endl
       << std::setfill('-') << std::setw(40) << '-' << "[Network]"
       << std::setfill('-') << std::setw(40) << '-'
       << "\nPublicIp:" << config.network_.public_ip
       << "\nPublicPort:" << config.network_.public_port
       << "\nPrivateIp:" << config.network_.private_ip
       << "\nPrivatePort:" << config.network_.private_port << std::endl
       << std::setfill('-') << std::setw(40) << '-' << "[Log]"
       << std::setfill('-') << std::setw(40) << '-'
       << "\nLevel:" << config.log_.level << "\nLogpath:" << config.log_.logpath
       << "\nPrefix:" << config.log_.prefix << "\nSuffix:" << config.log_.suffix
       << "\nMaxsize:" << config.log_.maxsize << std::endl
       << std::setfill('-') << std::setw(40) << '-' << "[Brpc]"
       << std::setfill('-') << std::setw(40) << '-'
       << "\nNumThreads:" << config.brpc_.num_threads
       << "\nMaxConcurrency:" << config.brpc_.max_concurrency << std::endl
       << std::setfill('-') << std::setw(40) << '-' << "[Etcd]"
       << std::setfill('-') << std::setw(40) << '-' << "\nInitialCluster"
       << config.etcd_.initial_cluster << std::endl
       << std::setfill('-') << std::setw(40) << '-' << "[Access]"
       << std::setfill('-') << std::setw(40) << '-' << std::endl
       << std::setfill('-') << std::setw(40) << '-' << "[SetManager]"
       << std::setfill('-') << std::setw(40) << '-'
       << "\nObsolete_interval_sec:" << config.setmanager_.obsolete_interval_sec
       << "\nNum_sets:" << config.setmanager_.num_sets << std::endl
       << std::setfill('-') << std::setw(40) << '-' << "[ExtentManager]"
       << std::setfill('-') << std::setw(40) << '-' << std::endl
       << std::setfill('-') << std::setw(40) << '-' << "[ExtentServer]"
       << std::setfill('-') << std::setw(40) << '-';

    return os;
}

}  // namespace common

common::Config &GlobalConfig() {
    static common::Config global_config;
    return global_config;
}

}  // namespace cyprestore
