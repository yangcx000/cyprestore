/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTMANAGER_EXTENT_ROUTER_H_
#define CYPRESTORE_EXTENTMANAGER_EXTENT_ROUTER_H_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cereal/access.hpp>
#include <cereal/types/string.hpp>
#include <mutex>
#include <sstream>
#include <string>

#include "common/constants.h"
#include "common/status.h"
#include "kvstore/rocks_store.h"
#include "utils/serializer.h"

namespace cyprestore {
namespace extentmanager {

class ExtentRouter {
public:
    ExtentRouter() = default;
    ExtentRouter(
            const std::string &extent_id, const std::string &pool_id,
            const std::string &rg_id, const std::string &blob_id);
    ~ExtentRouter() = default;

    std::string kv_key() {
        std::stringstream ss(
                common::kERKvPrefix, std::ios_base::app | std::ios_base::out);
        ss << "_" << extent_id_;
        return ss.str();
    }

private:
    friend class RouterManager;
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive &archive, const std::uint32_t version) {
        archive(extent_id_);
        archive(pool_id_);
        archive(rg_id_);
        archive(blob_id_);
        archive(create_time_);
        archive(update_time_);
    }
    std::string extent_id_;
    std::string pool_id_;
    std::string rg_id_;
    std::string blob_id_;
    std::string create_time_;
    std::string update_time_;
};

using common::Status;

class RouterManager {
public:
    RouterManager(std::shared_ptr<kvstore::RocksStore> kv_store);
    ~RouterManager() = default;

    Status query_router(
            const std::string &extent_id, std::string *pool_id,
            std::string *rg_id);
    // [begin, end] means extent index in a blob, eg: [0, 1023] count 1024
    // extents
    Status create_router(
            const std::string &pool_id, const std::string &blob_id, int begin,
            int end, const std::vector<std::string> &rg_ids);
    Status delete_router(const std::string &extent_id);

private:
    Status look_in_cache(
            const std::string &extent_id, std::string *pool_id,
            std::string *rg_id);
    void put_in_cache(
            const std::string &extent_id, std::shared_ptr<ExtentRouter> er);
    void delete_in_cache(const std::string &extent_id);

    std::shared_ptr<kvstore::RocksStore> kv_store_;
    std::map<
            int, std::unordered_map<std::string, std::shared_ptr<ExtentRouter>>>
            multi_extent_router_map_;
    std::unique_ptr<boost::shared_mutex[]> mutexs_;
};

}  // namespace extentmanager
}  // namespace cyprestore

CEREAL_CLASS_VERSION(cyprestore::extentmanager::ExtentRouter, 0);

#endif  // CYPRESTORE_EXTENTMANAGER_EXTENT_ROUTER_H_
