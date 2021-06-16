/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "extent_router.h"

#include "butil/logging.h"
#include "butil/time.h"
#include "bvar/bvar.h"
#include "common/config.h"
#include "common/error_code.h"
#include "common/extent_id_generator.h"
#include "utils/chrono.h"
#include "utils/hash.h"

namespace cyprestore {
namespace extentmanager {

bvar::LatencyRecorder g_latency_query("query_router");

ExtentRouter::ExtentRouter(
        const std::string &extent_id, const std::string &pool_id,
        const std::string &rg_id, const std::string &blob_id)
        : extent_id_(extent_id), pool_id_(pool_id), rg_id_(rg_id),
          blob_id_(blob_id) {
    create_time_ = utils::Chrono::DateString();
    update_time_ = create_time_;
}

RouterManager::RouterManager(std::shared_ptr<kvstore::RocksStore> kv_store)
        : kv_store_(kv_store) {
    int num = 1 << GlobalConfig().extentmanager().router_inst_shift;
    for (int i = 0; i < num; i++) {
        std::unordered_map<std::string, std::shared_ptr<ExtentRouter>> map;
        multi_extent_router_map_[i] = map;
    }
    mutexs_.reset(new boost::shared_mutex[num]);
}

Status RouterManager::create_router(
        const std::string &pool_id, const std::string &blob_id, int begin,
        int end, const std::vector<std::string> &rg_ids) {
    std::map<std::string, std::string> map;
    int index = 0;
    for (int i = begin; i <= end; i++) {
        auto id = common::ExtentIDGenerator::GenerateExtentID(blob_id, i);
        std::unique_ptr<ExtentRouter> router(
                new ExtentRouter(id, pool_id, rg_ids[index++], blob_id));
        std::string key = router->kv_key();
        std::string value =
                utils::Serializer<ExtentRouter>::Encode(*router.get());
        map[key] = value;
    }
    // router may be too many, so just store in kv, not in memory
    std::vector<std::pair<std::string, std::string>> kvs;
    for (auto &m : map) {
        kvs.push_back(std::make_pair(m.first, m.second));
        if (kvs.size() == 100) {
            auto status = kv_store_->MultiPut(kvs);
            if (!status.ok()) {
                LOG(ERROR) << "Persist extent router meta to store failed.";
                return Status(
                        common::CYPRE_EM_STORE_ERROR,
                        "persist extent router meta failed");
            }
            kvs.clear();
        }
    }

    if (!kvs.empty()) {
        auto status = kv_store_->MultiPut(kvs);
        if (!status.ok()) {
            LOG(ERROR) << "Persist extent router meta to store failed.";
            return Status(
                    common::CYPRE_EM_STORE_ERROR,
                    "persist extent router meta failed");
        }
    }
    return Status();
}

Status RouterManager::delete_router(const std::string &extent_id) {
    std::stringstream ss(
            common::kERKvPrefix, std::ios_base::app | std::ios_base::out);
    ss << "_" << extent_id;
    std::string key = ss.str();
    auto kv_status = kv_store_->Delete(key);
    if (!kv_status.ok()) {
        LOG(ERROR) << "Delete extent router meta from store failed.";
        return Status(
                common::CYPRE_EM_DELETE_ERROR,
                "delete extent router meta failed");
    }
    delete_in_cache(extent_id);
    return Status();
}

Status RouterManager::query_router(
        const std::string &extent_id, std::string *pool_id,
        std::string *rg_id) {
    butil::Timer timer;
    timer.start();
    // 1. look in cache
    auto ret = look_in_cache(extent_id, pool_id, rg_id);
    if (ret.ok()) {
        timer.stop();
        g_latency_query << timer.u_elapsed();
        return ret;
    }

    // 2. look in store
    std::stringstream ss(
            common::kERKvPrefix, std::ios_base::app | std::ios_base::out);
    ss << "_" << extent_id;
    std::string key = ss.str();
    std::string value;
    auto status = kv_store_->Get(key, &value);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            LOG(INFO) << "Extent router not exist, extent_id: " << extent_id;
            return Status(
                    common::CYPRE_EM_ROUTER_NOT_FOUND,
                    "Extent router not found.");
        }
        LOG(ERROR) << "Get extent router from store failed, extent_id: "
                   << extent_id;
        return Status(common::CYPRE_EM_LOAD_ERROR, "Get extent router failed.");
    }
    auto er = std::make_shared<ExtentRouter>();
    if (utils::Serializer<ExtentRouter>::Decode(value, *er.get())) {
        *rg_id = er->rg_id_;
        *pool_id = er->pool_id_;
        // 3. cache
        put_in_cache(extent_id, er);
        timer.stop();
        g_latency_query << timer.u_elapsed();
        return Status();
    }
    LOG(ERROR) << "Deocde extent router failed, extent_id: " << extent_id;
    return Status(
            common::CYPRE_EM_DECODE_ERROR, "Decode extent router meta failed");
}

Status RouterManager::look_in_cache(
        const std::string &extent_id, std::string *pool_id,
        std::string *rg_id) {
    unsigned int index = utils::HashUtil::mur_mur_hash(
            extent_id.c_str(), extent_id.length());
    int remain =
            index % (1 << GlobalConfig().extentmanager().router_inst_shift);
    auto multi_iter = multi_extent_router_map_.find(remain);

    boost::shared_lock<boost::shared_mutex> lock(mutexs_[remain]);
    auto iter = multi_iter->second.find(extent_id);
    if (iter != multi_iter->second.end()) {
        *pool_id = iter->second->pool_id_;
        *rg_id = iter->second->rg_id_;
        return Status();
    }
    return Status(common::CYPRE_EM_ROUTER_NOT_FOUND);
}

void RouterManager::put_in_cache(
        const std::string &extent_id, std::shared_ptr<ExtentRouter> er) {
    unsigned int index = utils::HashUtil::mur_mur_hash(
            extent_id.c_str(), extent_id.length());
    int remain =
            index % (1 << GlobalConfig().extentmanager().router_inst_shift);
    auto multi_iter = multi_extent_router_map_.find(remain);

    boost::unique_lock<boost::shared_mutex> lock(mutexs_[remain]);
    // double check
    auto iter = multi_iter->second.find(extent_id);
    if (iter != multi_iter->second.end()) {
        return;
    }
    multi_iter->second.insert(std::make_pair(extent_id, er));
}

void RouterManager::delete_in_cache(const std::string &extent_id) {
    unsigned int index = utils::HashUtil::mur_mur_hash(
            extent_id.c_str(), extent_id.length());
    int remain =
            index % (1 << GlobalConfig().extentmanager().router_inst_shift);
    auto multi_iter = multi_extent_router_map_.find(remain);

    boost::unique_lock<boost::shared_mutex> lock(mutexs_[remain]);
    // double check
    auto iter = multi_iter->second.find(extent_id);
    if (iter == multi_iter->second.end()) {
        return;
    }
    multi_iter->second.erase(iter);
}

}  // namespace extentmanager
}  // namespace cyprestore
