/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "extent_location.h"

#include <cereal/archives/binary.hpp>
#include <sstream>
#include <utility>

#include "butil/logging.h"
#include "extentserver.h"
#include "utils/coding.h"
#include "utils/crc32.h"
#include "utils/serializer.h"

namespace cyprestore {
namespace extentserver {

Status ExtentLocationMgr::Init(uint64_t disk_capacity) {
    space_alloc_.reset(new extentserver::SpaceAlloc(disk_capacity));
    if (space_alloc_->Init() != 0) {
        return Status(
                common::CYPRE_ES_INIT_SPACE_ALLOC_FAIL,
                "couldn't init space allocator");
    }

    kvstore::RocksOption rocks_option;
    rocks_option.db_path = GlobalConfig().rocksdb().db_path;
    rocks_option.compact_threads = GlobalConfig().rocksdb().compact_threads;
    rocks_store_.reset(new kvstore::RocksStore(rocks_option));
    auto status = rocks_store_->Open();
    if (!status.ok()) {
        LOG(ERROR) << "Couldn't open rocksdb "
                   << GlobalConfig().rocksdb().db_path << ", "
                   << status.ToString();
        return Status(common::CYPRE_ES_OPEN_ROCKSDB_ERROR, status.ToString());
    }

    return Status();
}

Status ExtentLocationMgr::Close() {
    auto s = rocks_store_->Close();
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't close rocksdb "
                   << GlobalConfig().rocksdb().db_path << ", " << s.ToString();
        return Status(common::CYPRE_ES_CLOSE_ROCKSDB_ERROR, s.ToString());
    }

    return Status();
}

void ExtentLocationMgr::addExtent(const ExtentLocationPtr &extent_loc) {
    common::WriteLock lock(lock_);
    extent_loc_map_.insert(std::make_pair(extent_loc->extent_id, extent_loc));
}

void ExtentLocationMgr::removeExtent(const std::string &extent_id) {
    common::WriteLock lock(lock_);
    extent_loc_map_.erase(extent_id);
}

Status ExtentLocationMgr::QueryLocation(
        const std::string &extent_id, Request *req, bool alloc_if_not_exists) {
    {
        common::ReadLock lock(lock_);
        auto it = extent_loc_map_.find(extent_id);
        if (it != extent_loc_map_.end()) {
            req->SetPhysicalOffset(it->second->offset + req->Offset());
            return Status();
        }
    }

    if (!alloc_if_not_exists) {
        return Status(
                common::CYPRE_ES_LOCATION_NOT_FOUND,
                "extent location not found");
    }
    // 锁住extent
    std::lock_guard<std::mutex> lock(extent_lock_mgr_.GetLock(extent_id));
    // 查询是否已经分配
    auto status = QueryLocation(extent_id, req, false);
    if (status.ok()) {
        return status;
    }

    std::unique_ptr<AUnit> aunit(new AUnit());
    status = space_alloc_->Allocate(extent_size_, &aunit);
    if (!status.ok()) {
        LOG(ERROR) << "Couldn't alloc space for " << extent_id << ", "
                   << status.ToString();
        return status;
    }

    auto extent_loc = std::make_shared<ExtentLocation>(
            aunit->offset, aunit->size, extent_id);
    status = persistExtent(extent_loc);
    if (!status.ok()) {
        LOG(ERROR) << "Couldn't alloc space for " << extent_id << ", "
                   << status.ToString();
        space_alloc_->Free(&aunit);
        return status;
    }
    req->SetPhysicalOffset(aunit->offset + req->Offset());
    return Status();
}

Status ExtentLocationMgr::persistExtent(const ExtentLocationPtr extent_loc) {
    std::string key = extent_loc->GenerateKey();
    std::string value = utils::Serializer<ExtentLocation>::Encode(*extent_loc);
    auto s = rocks_store_->Put(key, value);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't put extent " << extent_loc->extent_id
                   << " to rocks_store, " << s.ToString();
        return Status(
                common::CYPRE_ES_ROCKSDB_STORE_ERROR, "couldn't store extent");
    }

    addExtent(extent_loc);
    return Status();
}

ExtentLocationPtr ExtentLocationMgr::queryExtent(const std::string &extent_id) {
    common::ReadLock lock(lock_);
    auto it = extent_loc_map_.find(extent_id);
    if (it != extent_loc_map_.end()) {
        return it->second;
    }
    return nullptr;
}

Status ExtentLocationMgr::deleteExtent(const ExtentLocationPtr extent_loc) {
    std::string key = extent_loc->GenerateKey();
    auto s = rocks_store_->Delete(key);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't delete extent " << extent_loc->extent_id
                   << " from rocks_store, " << s.ToString();
        return Status(
                common::CYPRE_ES_ROCKSDB_STORE_ERROR, "couldn't delete extent");
    }

    removeExtent(extent_loc->extent_id);
    return Status();
}

Status ExtentLocationMgr::ReclaimExtent(const std::string &extent_id) {
    auto loc = queryExtent(extent_id);
    if (!loc) {
        return Status();
    }

    std::lock_guard<std::mutex> lock(extent_lock_mgr_.GetLock(extent_id));
    loc = queryExtent(extent_id);
    if (!loc) {
        return Status();
    }

    auto s = deleteExtent(loc);
    if (!s.ok()) {
        return s;
    }

    space_alloc_->Free(loc->offset, loc->size);
    return Status();
}

Status ExtentLocationMgr::LoadExtents() {
    std::unique_ptr<kvstore::KVIterator> kv_iter;
    kvstore::RocksStatus s =
            rocks_store_->ScanPrefix(kExtentLocPrefix, &kv_iter);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't Load extent locs, " << s.ToString();
        return Status(
                common::CYPRE_ES_ROCKSDB_LOAD_ERROR,
                "couldn't load extents from rocks store");
    }

    std::stringstream ss;
    while (kv_iter->Valid()) {
        ExtentLocation loc;
        if (!utils::Serializer<ExtentLocation>::Decode(kv_iter->value(), loc)) {
            LOG(ERROR) << "Couldn't Load extent locs";
            return Status(
                    common::CYPRE_ES_DECODE_ERROR,
                    "couldn't load extents from rocks store");
        }

        LOG(INFO) << "Load extent from rocksdb"
                  << ", extent_id:" << loc.extent_id
                  << ", offset:" << loc.offset << ", size:" << loc.size;

        ExtentLocationPtr extent_loc = std::make_shared<ExtentLocation>(loc);
        // 加入内存结构
        extent_loc_map_.insert(
                std::make_pair(extent_loc->extent_id, extent_loc));
        // 标记bitmap allocator
        space_alloc_->Mark(extent_loc->offset, extent_loc->size);

        kv_iter->Next();
    }

    LOG(INFO) << "Load extents finished";
    return Status();
}

}  // namespace extentserver
}  // namespace cyprestore
