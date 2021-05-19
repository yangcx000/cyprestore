/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "pool.h"

#include <memory>
#include <utility>

#include "blob.h"
#include "butil/logging.h"
#include "common/config.h"
#include "common/error_code.h"
#include "utils/chrono.h"
#include "utils/pb_transfer.h"
#include "utils/uuid.h"

namespace cyprestore {
namespace extentmanager {

bool replica_count_valid(int replica_count) {
    return replica_count > 0 && replica_count <= common::kMaxReplicaCount;
}

Pool::Pool()
        : id_(std::string()), type_(kPoolTypeUnknown),
          failure_domain_(kFailureDomainUnknown), status_(kPoolStatusUnknown),
          replica_num_(0), rg_num_(0), es_num_(0),
          extent_size_(common::kDefaultExtentSize), router_version_(0) {}

Pool::Pool(
        const std::string id, const std::string &name, PoolType type,
        PoolFailureDomain pfd, uint32_t replica_num, uint32_t es_num,
        uint64_t extent_size, std::shared_ptr<kvstore::RocksStore> kv_store)
        : id_(id), name_(name), type_(type), failure_domain_(pfd),
          status_(kPoolStatusCreated), replica_num_(replica_num),
          es_num_(es_num), extent_size_(extent_size) {
    rg_num_ = es_num_ * GlobalConfig().extentmanager().rg_per_es;
    router_version_ = 0;
    create_time_ = utils::Chrono::DateString();
    update_time_ = create_time_;
    blob_mgr_ = std::make_shared<BlobManager>(kv_store);
    es_mgr_ = std::make_shared<EsManager>(kv_store);
    rg_mgr_ = std::make_shared<RgManager>(kv_store);
}

common::pb::Pool Pool::ToPbPool() {
    common::pb::Pool pb_pool;
    pb_pool.set_id(id_);
    pb_pool.set_name(name_);
    pb_pool.set_type(utils::ToPbPoolType(type_));
    pb_pool.set_status(utils::ToPbPoolStatus(status_));
    pb_pool.set_replica_count(replica_num_);
    pb_pool.set_rg_count(rg_num_);
    pb_pool.set_es_count(es_num_);
    pb_pool.set_failure_domain(utils::ToPbFailureDomain(failure_domain_));
    pb_pool.set_create_date(create_time_);
    pb_pool.set_update_date(update_time_);
    pb_pool.set_capacity(total_capacity());
    pb_pool.set_size(total_used());
    pb_pool.set_extent_size(extent_size_);
    pb_pool.set_num_extents(total_nr_extents());

    return pb_pool;
}

/*
 * pool api
 */
Status PoolManager::init_pool(const std::string &pool_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    // 1. check pool exist
    if (iter == pool_map_.end()) {
        LOG(ERROR) << "Init pool failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }

    // 2. check pool status
    if (iter->second->status_ != PoolStatus::kPoolStatusCreated) {
        LOG(ERROR) << "Init pool failed, already init, pool: " << pool_id;
        return Status(
                common::CYPRE_EM_POOL_ALREADY_INIT, "pool already created");
    }

    // 3. check if extent server all report heartbeat
    auto es_count = iter->second->get_es_mgr()->es_count();
    if (iter->second->es_num_ != es_count) {
        LOG(ERROR) << "Init pool failed, not all es start up, expected count: "
                   << iter->second->es_num_ << ", actually count: " << es_count;
        return Status(common::CYPRE_EM_POOL_NOT_READY, "pool not ready");
    }

    // 3. query es
    std::vector<std::shared_ptr<ExtentServer>> ess;
    auto status = iter->second->get_es_mgr()->list_es(&ess);
    if (!status.ok()) {
        return status;
    }

    // 4. allocate rg
    status = iter->second->get_rg_mgr()->init(
            iter->second->id_, iter->second->replica_num_,
            iter->second->failure_domain_, ess);
    if (!status.ok()) {
        LOG(ERROR) << "Can not allocate replication group.";
        return status;
    }

    iter->second->status_ = PoolStatus::kPoolStatusEnabled;
    std::string value = utils::Serializer<Pool>::Encode(*iter->second.get());
    auto rock_status = kv_store_->Put(iter->second->kv_key(), value);
    if (!rock_status.ok()) {
        LOG(ERROR) << "Store pool meta to kv failed, " << status.ToString()
                   << ", pool_id:" << pool_id
                   << " pool_name:" << iter->second->name_;
        return Status(
                common::CYPRE_EM_STORE_ERROR, "failed to store pool meta");
    }

    LOG(INFO) << "Init pool secceed, pool id: " << pool_id;
    return Status();
}

void PoolManager::add_pool(std::unique_ptr<Pool> &&pool) {
    pool_map_[pool->id_] = std::move(pool);
}

bool PoolManager::pool_valid(const std::string &pool_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    return iter != pool_map_.end()
           && iter->second->status_ != kPoolStatusDisabled;
}

Status PoolManager::create_pool(
        const std::string &name, PoolType type, PoolFailureDomain pfd,
        uint32_t replica_num, uint32_t es_num, std::string *pool_id) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    for (auto &p : pool_map_) {
        if (p.second->name_ == name) {
            LOG(ERROR) << "Duplicated pool name, pool_name:" << name;
            return Status(
                    common::CYPRE_EM_POOL_DUPLICATED, "duplicated pool name");
        }
    }

    std::string id = allocate_poolid();
    if (id.empty()) {
        LOG(ERROR) << "Allocate pool id failed, may exceed max pool nums.";
        return Status(
                common::CYPRE_EM_POOL_ALLOCATE_FAIL, "allocate pool id failed");
    }
    std::unique_ptr<Pool> pool(new Pool(
            id, name, type, pfd, replica_num, es_num, extent_size_, kv_store_));

    std::string value = utils::Serializer<Pool>::Encode(*pool.get());
    auto status = kv_store_->Put(pool->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "Store pool meta to kv failed, " << status.ToString()
                   << ", pool_id:" << pool->id_ << " pool_name:" << pool->name_;
        return Status(
                common::CYPRE_EM_STORE_ERROR, "failed to store pool meta");
    }

    add_pool(std::move(pool));
    *pool_id = id;
    LOG(INFO) << "Create pool secceed, pool id: " << id << ", name: " << name
              << ", poolType: " << type << ", failureDomain: " << pfd
              << ", replicaNum: " << replica_num << ", esNum: " << es_num
              << ", extentSize: " << extent_size_;
    return Status();
}

Status PoolManager::update_pool(
        const std::string &pool_id, const std::string &name, PoolType type,
        PoolFailureDomain fd, uint32_t replica_num, uint32_t es_num) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    for (auto &p : pool_map_) {
        if (p.second->id_ != pool_id && p.second->name_ == name) {
            LOG(ERROR) << "Duplicated pool name, pool_name:" << name;
            return Status(
                    common::CYPRE_EM_POOL_DUPLICATED, "duplicated pool name");
        }
    }
    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        LOG(ERROR) << "Update pool failed, pool not found, pool_id:" << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "pool not found");
    }
    iter->second->name_ = name.empty() ? iter->second->name_ : name;
    iter->second->type_ = type == kPoolTypeUnknown ? iter->second->type_ : type;
    iter->second->failure_domain_ =
            fd == kFailureDomainUnknown ? iter->second->failure_domain_ : fd;
    // TODO need to check replica_num < es_num
    iter->second->replica_num_ =
            replica_num == 0 ? iter->second->replica_num_ : replica_num;
    iter->second->es_num_ = es_num == 0 ? iter->second->es_num_ : es_num;

    std::string value = utils::Serializer<Pool>::Encode(*iter->second.get());
    auto status = kv_store_->Put(iter->second->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "Store pool meta to kv failed, " << status.ToString()
                   << ", pool_id:" << pool_id;
        return Status(
                common::CYPRE_EM_STORE_ERROR, "failed to store pool meta");
    }

    LOG(INFO) << "Update pool secceed, pool id: " << pool_id;
    return Status();
}

Status
PoolManager::query_pool(const std::string &pool_id, common::pb::Pool *pool) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        LOG(ERROR) << "Not found pool, pool id: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    *pool = iter->second->ToPbPool();
    return Status();
}

std::string PoolManager::allocate_poolid() {
    for (char index = 'a'; index <= 'z'; index++) {
        std::stringstream pool_id(
                common::kPoolIdPrefix, std::ios_base::app | std::ios_base::out);
        pool_id << index;
        if (pool_map_.find(pool_id.str()) == pool_map_.end()) {
            return pool_id.str();
        }
    }
    return std::string();
}

uint32_t PoolManager::get_router_version(const std::string &pool_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        return -1;
    }
    return iter->second->router_version_;
}

Status PoolManager::soft_delete_pool(const std::string &pool_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        LOG(ERROR) << "Delete pool failed, not find pool, pool_id:" << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }

    iter->second->status_ = kPoolStatusDisabled;
    std::string value = utils::Serializer<Pool>::Encode(*iter->second.get());
    auto status = kv_store_->Put(iter->second->kv_key(), value);
    if (!status.ok()) {
        LOG(ERROR) << "delete pool meta from store failed, "
                   << status.ToString() << ", pool_id:" << pool_id;
        return Status(
                common::CYPRE_EM_STORE_ERROR, "failed to delete pool meta");
    }
    return Status();
}

Status PoolManager::delete_pool(const std::string &pool_id) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()) {
        LOG(ERROR) << "Delete pool failed, not find pool, pool_id:" << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }

    auto status = kv_store_->Delete(iter->second->kv_key());
    if (!status.ok()) {
        LOG(ERROR) << "delete pool meta from store failed, "
                   << status.ToString() << ", pool_id:" << pool_id;
        return Status(
                common::CYPRE_EM_DELETE_ERROR, "failed to delete pool meta");
    }
    pool_map_.erase(iter);
    return Status();
}

bool PoolManager::recovery_from_store() {
    // 1. recover pool meta first
    std::unique_ptr<kvstore::KVIterator> iter;
    auto status = kv_store_->ScanPrefix(common::kPoolKvPrefix, &iter);
    if (!status.ok()) {
        LOG(ERROR) << "Scan extent router meta from store failed, status: "
                   << status.ToString();
        return false;
    }
    std::vector<std::string> tmp;
    while (iter->Valid()) {
        tmp.push_back(iter->value());
        iter->Next();
    }
    for (auto &v : tmp) {
        std::unique_ptr<Pool> p(new Pool());
        if (utils::Serializer<Pool>::Decode(v, *p.get())) {
            p->es_mgr_ = std::make_shared<EsManager>(kv_store_);
            p->blob_mgr_ = std::make_shared<BlobManager>(kv_store_);
            p->rg_mgr_ = std::make_shared<RgManager>(kv_store_);
            pool_map_[p->id_] = std::move(p);
        } else {
            LOG(ERROR) << "Decode pool meta failed.";
            return false;
        }
    }
    LOG(INFO) << "Recover pool meta succedd, include: " << pool_map_.size()
              << " pools.";
    // 2. recover es, blob, rg for every pool
    for (auto &p : pool_map_) {
        auto success = p.second->get_es_mgr()->recovery_from_store(p.first);
        if (!success) {
            return success;
        }
        p.second->get_rg_mgr()->recovery_from_store(p.first);
        if (!success) {
            return success;
        }
        success = p.second->get_blob_mgr()->recovery_from_store(p.first);
        if (!success) {
            return success;
        }
    }
    return true;
}

std::string PoolManager::allocate_pool(BlobType type) {
    std::string pool_id;
    switch (type) {
        case BlobType::kBlobTypeGeneral:
            return get_pool(kPoolTypeHDD);
        case BlobType::kBlobTypeSuperior:
            return get_pool(kPoolTypeSSD);
        case BlobType::kBlobTypeExcellent:
            return get_pool(kPoolTypeNVMeSSD);
        default:
            return get_pool(kPoolTypeHDD);
    }
}

std::string PoolManager::get_pool(PoolType type) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    double min = 1.0;
    std::string pool_id;
    for (auto &p : pool_map_) {
        if (p.second->type_ == type
            && p.second->status_ == kPoolStatusEnabled) {
            auto ratio =
                    p.second->total_used() / (double)p.second->total_capacity();
            if (ratio < min) {
                min = ratio;
                pool_id = p.second->id_;
            }
        }
    }
    return pool_id;
}

Status PoolManager::list_pools(std::vector<common::pb::Pool> *pools) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &p : pool_map_) {
        if (p.second->status_ == kPoolStatusEnabled) {
            pools->push_back(p.second->ToPbPool());
        }
    }
    return Status();
}

Status PoolManager::list_pools(std::vector<std::string> *pools) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &p : pool_map_) {
        if (p.second->status_ == kPoolStatusEnabled) {
            pools->push_back(p.first);
        }
    }
    return Status();
}

Status PoolManager::list_deleted_pools(std::vector<std::string> *pools) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &p : pool_map_) {
        if (p.second->status_ == kPoolStatusDisabled) {
            pools->push_back(p.first);
        }
    }
    return Status();
}

/*
 * blob api
 */

Status PoolManager::create_blob(
        const std::string &pool_id, const std::string &blob_name, uint64_t size,
        BlobType type, const std::string &desc, const std::string &user_id,
        const std::string &instance_id, std::string *blob_id,
        std::vector<std::string> *rg_ids) {
    // 1. generate blob id
    std::string id = allocate_blobid();
    *blob_id = id;

    // 2. create blob info
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ != kPoolStatusEnabled) {
        LOG(ERROR) << "Create blob failed, not find pool";
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not find pool");
    }
    auto status = iter->second->get_blob_mgr()->create_blob(
            id, blob_name, size, type, desc, pool_id, user_id, instance_id);
    if (!status.ok()) {
        return status;
    }

    // 3. allocate replication groups
    int extent_num = int(size / extent_size_);
    std::vector<std::shared_ptr<ExtentServer>> ess;
    iter->second->get_es_mgr()->list_es(&ess);

    return iter->second->get_rg_mgr()->allocate_rgs(extent_num, ess, rg_ids);
}

Status PoolManager::resize_blob(
        const std::string &user_id, const std::string &blob_id,
        uint64_t new_size, std::string *pool_id, uint64_t *old_size,
        std::vector<std::string> *rg_ids) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    // 1. resize blob
    for (auto &p : pool_map_) {
        if (p.second->get_blob_mgr()->blob_valid(blob_id)) {
            auto status = p.second->get_blob_mgr()->resize_blob(
                    user_id, blob_id, new_size, old_size);
            if (!status.ok()) {
                return status;
            } else {
                int extent_num = int((new_size - *old_size) / extent_size_);
                std::vector<std::shared_ptr<ExtentServer>> ess;
                p.second->get_es_mgr()->list_es(&ess);
                *pool_id = p.first;
                status = p.second->get_rg_mgr()->allocate_rgs(
                        extent_num, ess, rg_ids);
                return status;
            }
        }
    }

    LOG(ERROR) << "Resize blob failed, not find blob";
    return Status(common::CYPRE_EM_BLOB_NOT_FOUND, "not find blob");
}

Status PoolManager::soft_delete_blob(
        const std::string &user_id, const std::string &blob_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &p : pool_map_) {
        if (p.second->get_blob_mgr()->blob_valid(blob_id)) {
            return p.second->get_blob_mgr()->soft_delete_blob(user_id, blob_id);
        }
    }
    LOG(ERROR) << "Delete blob failed, not find blob";
    return Status(common::CYPRE_EM_BLOB_NOT_FOUND, "not find blob");
}

Status PoolManager::delete_blob(
        const std::string &pool_id, const std::string &blob_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()) {
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not find pool");
    }
    return iter->second->get_blob_mgr()->delete_blob(blob_id);
}

Status PoolManager::rename_blob(
        const std::string &user_id, const std::string &blob_id,
        const std::string &new_name) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &p : pool_map_) {
        if (p.second->get_blob_mgr()->blob_valid(blob_id)) {
            return p.second->get_blob_mgr()->rename_blob(
                    user_id, blob_id, new_name);
        }
    }
    LOG(ERROR) << "Rename blob failed, not find blob";
    return Status(common::CYPRE_EM_BLOB_NOT_FOUND, "not find blob");
}

Status PoolManager::query_blob(
        const std::string &blob_id, bool all, common::pb::Blob *blob) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &p : pool_map_) {
        if (p.second->get_blob_mgr()->blobid_exist(blob_id)) {
            return p.second->get_blob_mgr()->query_blob(blob_id, all, blob);
        }
    }
    LOG(ERROR) << "Query blob failed, not find blob";
    return Status(common::CYPRE_EM_BLOB_NOT_FOUND, "not find blob");
}

Status PoolManager::list_blobs(
        const std::string &user_id, std::vector<common::pb::Blob> *blobs) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &p : pool_map_) {
        if (p.second->status_ == kPoolStatusDisabled) {
            continue;
        }
        std::vector<common::pb::Blob> res;
        auto status = p.second->get_blob_mgr()->list_blobs(user_id, &res);
        if (status.ok()) {
            blobs->insert(blobs->end(), res.begin(), res.end());
        }
    }
    return Status();
}

Status PoolManager::list_blobs(
        const std::string &pool_id, const std::string &user_id,
        std::vector<std::string> *blobs) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    return iter->second->get_blob_mgr()->list_blobs(user_id, blobs);
}

Status PoolManager::list_blobs(
        const std::string &pool_id, bool deleted,
        std::vector<std::string> *blobs) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()) {
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not find pool");
    }
    return iter->second->get_blob_mgr()->list_blobs(deleted, blobs);
}

std::string PoolManager::allocate_blobid() {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    std::string blob_id;
    do {
        blob_id = common::kBlobIdPrefix + utils::UUID::Generate();
        if (!blobid_exist(blob_id)) {
            return blob_id;
        }
    } while (true);
}

bool PoolManager::blobid_exist(const std::string &blob_id) {
    for (auto &p : pool_map_) {
        if (p.second->get_blob_mgr()->blobid_exist(blob_id)) {
            return true;
        }
    }
    return false;
}

bool PoolManager::blob_valid(
        const std::string &pool_id, const std::string &blob_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()) {
        return false;
    }
    return iter->second->get_blob_mgr()->blob_valid(blob_id);
}

/*
 * extent server api
 */
bool PoolManager::es_exist(const std::string &pool_id, int id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        return false;
    }
    return iter->second->get_es_mgr()->es_exist(id);
}

Status PoolManager::query_es(
        const std::string &pool_id, int id, common::pb::ExtentServer *es) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        LOG(ERROR) << "query es failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    return iter->second->get_es_mgr()->query_es(id, es);
}

Status PoolManager::delete_es(const std::string &pool_id, int id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ != kPoolStatusCreated) {
        LOG(ERROR) << "query es failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    return iter->second->get_es_mgr()->delete_es(id);
}

Status PoolManager::delete_all_es(const std::string &pool_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()) {
        LOG(ERROR) << "query es failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    return iter->second->get_es_mgr()->delete_all_es();
}

Status PoolManager::update_es(
        const std::string &pool_id, int id, const std::string &name,
        const std::string &public_ip, int public_port,
        const std::string &private_ip, int private_port, uint64_t size) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        LOG(ERROR) << "query es failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    return iter->second->get_es_mgr()->update_es(
            id, name, public_ip, public_port, private_ip, private_port, size);
}

Status PoolManager::list_es(
        const std::string &pool_id,
        std::vector<common::pb::ExtentServer> *ess) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        LOG(ERROR) << "query es failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    return iter->second->get_es_mgr()->list_es(ess);
}

Status PoolManager::list_es(
        const std::string &pool_id,
        std::vector<std::shared_ptr<ExtentServer>> *ess) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ == kPoolStatusDisabled) {
        LOG(ERROR) << "query es failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    return iter->second->get_es_mgr()->list_es(ess);
}

Status PoolManager::create_es(
        int id, const std::string &name, const std::string &public_ip,
        int public_port, const std::string &private_ip, int private_port,
        uint64_t size, uint64_t capacity, const std::string &pool_id,
        const std::string &host, const std::string &rack) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ != kPoolStatusCreated) {
        LOG(ERROR) << "query es failed, not found pool or pool busy: "
                   << pool_id;
        return Status(
                common::CYPRE_EM_POOL_NOT_FOUND, "not found pool or pool busy");
    }
    return iter->second->get_es_mgr()->create_es(
            id, name, public_ip, public_port, private_ip, private_port, size,
            capacity, pool_id, host, rack);
}

Status PoolManager::query_es_router(
        const std::string &pool_id, const std::vector<int> &es_group,
        common::pb::ExtentRouter *router) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ != kPoolStatusEnabled) {
        LOG(ERROR) << "query es failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }

    return iter->second->get_es_mgr()->query_es_router(es_group, router);
}

/*
 * replication group api
 */
Status PoolManager::query_rg(
        const std::string &pool_id, const std::string &rg_id,
        common::pb::ReplicaGroup *rg) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ != kPoolStatusEnabled) {
        LOG(ERROR) << "query rg failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    return iter->second->get_rg_mgr()->query_rg(rg_id, rg);
}

Status PoolManager::list_rg(
        const std::string &pool_id,
        std::vector<common::pb::ReplicaGroup> *rgs) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    if (iter == pool_map_.end()
        || iter->second->status_ != kPoolStatusEnabled) {
        LOG(ERROR) << "query rg failed, not found pool: " << pool_id;
        return Status(common::CYPRE_EM_POOL_NOT_FOUND, "not found pool");
    }
    return iter->second->get_rg_mgr()->list_rg(rgs);
}

Status PoolManager::delete_all_rg(const std::string &pool_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = pool_map_.find(pool_id);
    return iter->second->get_rg_mgr()->delete_all_rg();
}

std::vector<int> PoolManager::query_es_group(
        const std::string &pool_id, const std::string &rg_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    // TODO add status
    auto iter = pool_map_.find(pool_id);
    return iter->second->get_rg_mgr()->query_es_group(rg_id);
}

}  // namespace extentmanager
}  // namespace cyprestore
