/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTSERVER_EXTENT_LOCATION_H_
#define CYPRESTORE_EXTENTSERVER_EXTENT_LOCATION_H_

#include <butil/macros.h>

#include <cereal/types/string.hpp>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "common/rwlock.h"
#include "common/status.h"
#include "kvstore/rocks_store.h"
#include "request_context.h"
#include "space_alloc.h"

namespace cyprestore {
namespace extentserver {

struct ExtentLocation;
class ExtentLocationMgr;

typedef std::shared_ptr<ExtentLocation> ExtentLocationPtr;
typedef std::unordered_map<std::string, ExtentLocationPtr> ExtentLocationMap;
typedef std::shared_ptr<ExtentLocationMgr> ExtentLocationMgrPtr;

const std::string kExtentLocPrefix = "extent_loc_";

struct ExtentLocation {
    ExtentLocation() {}
    ExtentLocation(
            uint64_t offset_, uint64_t size_, const std::string &extent_id_)
            : offset(offset_), size(size_), extent_id(extent_id_) {}

    std::string GenerateKey() {
        std::stringstream ss;
        ss << kExtentLocPrefix << extent_id;
        return ss.str();
    }

    // cereal序列化和反序列化函数
    template <class Archive> void serialize(Archive &archive) {
        archive(offset);
        archive(size);
        archive(extent_id);
    }

    uint64_t offset;
    uint64_t size;
    std::string extent_id;
    // TODO: 补充其它属性
};

using common::Status;

class ExtentLocationMgr {
public:
    ExtentLocationMgr() = default;
    ~ExtentLocationMgr() = default;

    Status Init(uint64_t disk_capacity);
    Status Close();
    Status QueryLocation(
            const std::string &extent_id, Request *req,
            bool alloc_if_not_exists = true);
    Status ReclaimExtent(const std::string &extent_id);
    Status LoadExtents();

    void SetExtentSize(uint64_t extent_size) {
        extent_size_ = extent_size;
    }
    uint64_t UsedSize() {
        return space_alloc_->UsedSize();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ExtentLocationMgr);

    void addExtent(const ExtentLocationPtr &extent_loc);
    void removeExtent(const std::string &extent_id);
    ExtentLocationPtr queryExtent(const std::string &extent_id);
    Status persistExtent(const ExtentLocationPtr extent_loc);
    Status deleteExtent(const ExtentLocationPtr extent_loc);

    uint64_t extent_size_;
    kvstore::RocksStorePtr rocks_store_;
    SpaceAllocPtr space_alloc_;
    common::ExtentLockMgr extent_lock_mgr_;
    ExtentLocationMap extent_loc_map_;
    common::RWLock lock_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_EXTENT_LOCATION_H_
