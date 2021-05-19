/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_POOL_H_
#define CYPRESTORE_EXTENTMANAGER_POOL_H_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cereal/access.hpp>
#include <cereal/types/string.hpp>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "blob.h"
#include "common/constants.h"
#include "common/pb/types.pb.h"
#include "common/status.h"
#include "enum.h"
#include "extent_server.h"
#include "kvstore/rocks_store.h"
#include "replication_group.h"
#include "utils/serializer.h"

namespace cyprestore {
namespace extentmanager {

bool replica_count_valid(int replica_count);

class Pool {
public:
    Pool();
    Pool(const std::string id, const std::string &name, PoolType type,
         PoolFailureDomain fd, uint32_t replica_num, uint32_t es_num,
         uint64_t extent_size, std::shared_ptr<kvstore::RocksStore> kv_store);
    ~Pool() = default;

    std::string kv_key() {
        std::stringstream ss(
                common::kPoolKvPrefix, std::ios_base::app | std::ios_base::out);
        ss << "_" << id_;
        return ss.str();
    }
    common::pb::Pool ToPbPool();
    uint64_t total_capacity() {
        return es_mgr_->total_capacity();
    }
    uint64_t total_used() {
        return es_mgr_->total_used();
    }
    uint64_t total_nr_extents() {
        return rg_mgr_->total_nr_extents();
    }

    void set_kv_store(std::shared_ptr<kvstore::RocksStore> kv_store) {
        kv_store_ = kv_store;
    }

    std::shared_ptr<BlobManager> get_blob_mgr() {
        return blob_mgr_;
    }
    std::shared_ptr<EsManager> get_es_mgr() {
        return es_mgr_;
    }
    std::shared_ptr<RgManager> get_rg_mgr() {
        return rg_mgr_;
    }

private:
    friend class PoolManager;
    friend class cereal::access;
    // TODO for compatible, cereal my use version info, detail can be found in
    // official website
    template <class Archive>
    void serialize(Archive &archive, const std::uint32_t version) {
        archive(id_);
        archive(name_);
        archive(type_);
        archive(failure_domain_);
        archive(status_);
        archive(replica_num_);
        archive(rg_num_);
        archive(es_num_);
        archive(extent_size_);
        archive(create_time_);
        archive(update_time_);
        if (version > 0) {
            archive(router_version_);
        }
    }

    std::string id_;
    std::string name_;
    PoolType type_;
    PoolFailureDomain failure_domain_;
    PoolStatus status_;
    uint32_t replica_num_;
    uint32_t rg_num_;
    uint32_t es_num_;
    uint64_t extent_size_;
    // extent router has one version for all
    uint32_t router_version_;
    std::string create_time_;
    std::string update_time_;

    std::shared_ptr<kvstore::RocksStore> kv_store_;
    std::shared_ptr<RgManager> rg_mgr_;
    std::shared_ptr<BlobManager> blob_mgr_;
    std::shared_ptr<EsManager> es_mgr_;
};

using common::Status;

class PoolManager {
public:
    PoolManager(
            std::shared_ptr<kvstore::RocksStore> kv_store, uint64_t extent_size)
            : kv_store_(kv_store), extent_size_(extent_size) {}
    ~PoolManager() = default;

    // pool
    Status create_pool(
            const std::string &name, PoolType type, PoolFailureDomain fd,
            uint32_t replica_num, uint32_t es_num, std::string *pool_id);
    Status init_pool(const std::string &pool_id);
    Status update_pool(
            const std::string &pool_id, const std::string &name, PoolType type,
            PoolFailureDomain fd, uint32_t replica_num, uint32_t es_num);
    Status delete_pool(const std::string &pool_id);
    Status soft_delete_pool(const std::string &pool_id);
    Status query_pool(const std::string &pool_id, common::pb::Pool *pool);
    Status list_pools(std::vector<common::pb::Pool> *pools);
    Status list_pools(std::vector<std::string> *pools);
    Status list_deleted_pools(std::vector<std::string> *pools);
    std::string allocate_pool(BlobType type);
    uint32_t get_router_version(const std::string &pool_id);
    bool pool_valid(const std::string &pool_id);
    bool recovery_from_store();

    // blob
    Status create_blob(
            const std::string &pool_id, const std::string &blob_name,
            uint64_t size, BlobType type, const std::string &desc,
            const std::string &user_id, const std::string &instance_id,
            std::string *blob_id, std::vector<std::string> *rg_ids);
    Status
    soft_delete_blob(const std::string &user_id, const std::string &blob_id);
    Status delete_blob(const std::string &pool_id, const std::string &blob_id);
    Status rename_blob(
            const std::string &user_id, const std::string &blob_id,
            const std::string &new_name);
    Status resize_blob(
            const std::string &user_id, const std::string &blob_id,
            uint64_t new_size, std::string *pool_id, uint64_t *old_size,
            std::vector<std::string> *rg_ids);
    Status
    query_blob(const std::string &blob_id, bool all, common::pb::Blob *blob);
    Status list_blobs(
            const std::string &user_id, std::vector<common::pb::Blob> *blobs);
    Status list_blobs(
            const std::string &pool_id, const std::string &user_id,
            std::vector<std::string> *blobs);
    Status list_blobs(
            const std::string &pool_id, bool deleted,
            std::vector<std::string> *blobs);
    bool blob_valid(const std::string &pool_id, const std::string &blob_id);

    // extent server
    bool es_exist(const std::string &pool_id, int id);
    Status
    query_es(const std::string &pool_id, int id, common::pb::ExtentServer *es);
    Status delete_es(const std::string &pool_id, int id);
    Status delete_all_es(const std::string &pool_id);
    Status update_es(
            const std::string &pool_id, int id, const std::string &name,
            const std::string &public_ip, int public_port,
            const std::string &private_ip, int private_port, uint64_t size);
    Status create_es(
            int id, const std::string &name, const std::string &public_ip,
            int public_port, const std::string &private_ip, int private_port,
            uint64_t size, uint64_t capacity, const std::string &pool_id,
            const std::string &host, const std::string &rack);
    Status
    list_es(const std::string &pool_id,
            std::vector<common::pb::ExtentServer> *ess);
    Status query_es_router(
            const std::string &pool_id, const std::vector<int> &es_group,
            common::pb::ExtentRouter *router);

    // replication group
    Status query_rg(
            const std::string &pool_id, const std::string &rg_id,
            common::pb::ReplicaGroup *rg);
    Status
    list_rg(const std::string &pool_id,
            std::vector<common::pb::ReplicaGroup> *rgs);
    Status delete_all_rg(const std::string &pool_id);
    std::vector<int>
    query_es_group(const std::string &pool_id, const std::string &rg_id);

    uint64_t get_extent_size() {
        return extent_size_;
    }

private:
    void add_pool(std::unique_ptr<Pool> &&pool);
    std::string allocate_poolid();
    std::string allocate_blobid();
    bool blobid_exist(const std::string &blob_id);
    std::string get_pool(PoolType type);
    Status
    list_es(const std::string &pool_id,
            std::vector<std::shared_ptr<ExtentServer>> *ess);
    // pool_id -> Pool
    std::map<std::string, std::unique_ptr<Pool>> pool_map_;
    boost::shared_mutex mutex_;

    std::shared_ptr<kvstore::RocksStore> kv_store_;
    // for all pool in one extentmanager, only exist one kind of extent size
    uint64_t extent_size_;
};

}  // namespace extentmanager
}  // namespace cyprestore

CEREAL_CLASS_VERSION(cyprestore::extentmanager::Pool, 1);

#endif  // CYPRESTORE_EXTENTMANAGER_POOL_H_
