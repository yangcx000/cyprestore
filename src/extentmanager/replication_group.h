/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_REPLICATION_GROUP_H_
#define CYPRESTORE_EXTENTMANAGER_REPLICATION_GROUP_H_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cereal/access.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/constants.h"
#include "common/pb/types.pb.h"
#include "common/status.h"
#include "enum.h"
#include "extent_server.h"
#include "kvstore/rocks_store.h"
#include "rg_allocater.h"
#include "utils/serializer.h"

namespace cyprestore {
namespace extentmanager {

class ReplicationGroup {
public:
    ReplicationGroup() = default;
    ReplicationGroup(const std::string &id, const std::string &pool_id)
            : id_(id), status_(kRGStatusOk), nr_extents_(0), pool_id_(pool_id) {
        create_time_ = utils::Chrono::DateString();
        update_time_ = create_time_;
    }
    ~ReplicationGroup() = default;

    std::string kv_key() {
        std::stringstream ss(
                common::kRGKvPrefix, std::ios_base::app | std::ios_base::out);
        ss << "_" << pool_id_ << "_" << id_;
        return ss.str();
    }
    common::pb::ReplicaGroup ToPbReplicationGroup();
    uint64_t get_nr_extents() {
        return nr_extents_;
    }
    void add_nr_extents() {
        nr_extents_++;
    }
    std::vector<int> &get_extent_servers() {
        return extent_servers_;
    }

private:
    friend class RgManager;
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive &archive, const std::uint32_t version) {
        archive(id_);
        archive(status_);
        archive(nr_extents_);
        archive(pool_id_);
        archive(create_time_);
        archive(update_time_);
        archive(extent_servers_);
    }

    std::string id_;
    RGStatus status_;
    uint64_t nr_extents_;
    std::string pool_id_;
    std::string create_time_;
    std::string update_time_;
    // vector<es_id>
    std::vector<int> extent_servers_;
};

typedef std::shared_ptr<ReplicationGroup> RGPtr;

struct RGCmp {
    bool operator()(const RGPtr &lhs, const RGPtr &rhs) {
        return lhs->get_nr_extents() > rhs->get_nr_extents();
    }
};

typedef std::priority_queue<RGPtr, std::vector<RGPtr>, RGCmp> RGHeap;

// each es may have a lot replication group that it is primary, all thie es rg
// compose a EsRg
struct EsRg {
    EsRg(int id_) : id(id_), weight(0), nr_extents(0) {}

    int id;
    int weight;
    uint64_t nr_extents;
    RGHeap rg_heap;
};
typedef std::shared_ptr<EsRg> EsRgPtr;

struct EsRgCmp {
    bool operator()(const EsRgPtr &lhs, const EsRgPtr &rhs) {
        return 10000 * lhs->nr_extents / lhs->weight
               > 10000 * rhs->nr_extents / rhs->weight;
    }
};
typedef std::priority_queue<EsRgPtr, std::vector<EsRgPtr>, EsRgCmp> EsRgHeap;

using common::Status;

class RgManager {
public:
    RgManager(std::shared_ptr<kvstore::RocksStore> kv_store)
            : kv_store_(kv_store){};
    ~RgManager() = default;

    Status
    init(const std::string &pool_id, int replica_count, PoolFailureDomain fd,
         const std::vector<std::shared_ptr<ExtentServer>> &ess);

    Status query_rg(const std::string &rg_id, common::pb::ReplicaGroup *es);
    Status allocate_rgs(
            int extent_num,
            const std::vector<std::shared_ptr<ExtentServer>> &ess,
            std::vector<std::string> *rg_ids);
    Status list_rg(std::vector<common::pb::ReplicaGroup> *rgs);
    Status delete_all_rg();
    std::vector<int> query_es_group(const std::string &rg_id);
    uint32_t rg_count() {
        return rg_map_.size();
    }
    bool nearfull(
            const std::string &rg_id,
            const std::vector<std::shared_ptr<ExtentServer>> &ess);
    bool recovery_from_store(const std::string &pool_id);
    uint64_t total_nr_extents();

private:
    std::string
    allocate_rg(const std::vector<std::shared_ptr<ExtentServer>> &ess);
    void build_heap();

    // rg_id -> ReplicationGroup
    std::unordered_map<std::string, RGPtr> rg_map_;
    EsRgHeap es_rg_heap_;
    boost::shared_mutex mutex_;
    std::shared_ptr<RGAllocater> rg_allocater_;
    std::shared_ptr<kvstore::RocksStore> kv_store_;
};

}  // namespace extentmanager
}  // namespace cyprestore

CEREAL_CLASS_VERSION(cyprestore::extentmanager::ReplicationGroup, 0);

#endif  // CYPRESTORE_EXTENTMANAGER_REPLICATION_GROUP_H_
