/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "replication_group.h"

#include <list>
#include <set>

#include "butil/logging.h"
#include "common/config.h"
#include "common/error_code.h"
#include "rg_allocater_by_es.h"
#include "rg_allocater_by_host.h"
#include "rg_allocater_by_rack.h"
#include "utils/chrono.h"
#include "utils/pb_transfer.h"

namespace cyprestore {
namespace extentmanager {

common::pb::ReplicaGroup ReplicationGroup::ToPbReplicationGroup() {
    common::pb::ReplicaGroup pb_rg;

    pb_rg.set_id(id_);
    pb_rg.set_status(utils::ToPbRgStatus(status_));

    pb_rg.set_extent_size(GlobalConfig().extentmanager().extent_size);
    pb_rg.set_nr_extents(nr_extents_);
    pb_rg.set_pool_id(pool_id_);
    pb_rg.set_create_date(create_time_);
    pb_rg.set_update_date(update_time_);
    for (uint32_t i = 0; i < extent_servers_.size(); i++) {
        pb_rg.add_extent_servers(extent_servers_[i]);
    }
    return pb_rg;
}

Status RgManager::init(
        const std::string &pool_id, int replica_count, PoolFailureDomain fd,
        const std::vector<std::shared_ptr<ExtentServer>> &ess) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    switch (fd) {
        case PoolFailureDomain::kFailureDomainRack:
            rg_allocater_ =
                    std::shared_ptr<RGAllocaterByRack>(new RGAllocaterByRack());
            break;
        case PoolFailureDomain::kFailureDomainHost:
            rg_allocater_ =
                    std::shared_ptr<RGAllocaterByHost>(new RGAllocaterByHost());
            break;
        case PoolFailureDomain::kFailureDomainNode:
            rg_allocater_ =
                    std::shared_ptr<RGAllocaterByEs>(new RGAllocaterByEs());
            break;
        default:
            return Status(
                    common::CYPRE_ER_INVALID_ARGUMENT,
                    "failuredomain not defined");
    }
    auto status = rg_allocater_->init(
            ess, GlobalConfig().extentmanager().extent_size);
    if (!status.ok()) {
        return status;
    }

    int rg_total = GlobalConfig().extentmanager().rg_per_es * ess.size();
    for (int i = 0; i < rg_total; i++) {
        std::string rg_id = pool_id + "." + std::to_string(i);
        rg_map_[rg_id] = std::make_shared<ReplicationGroup>(rg_id, pool_id);
    }

    // allocate primary
    for (auto &rg : rg_map_) {
        auto es = rg_allocater_->select_primary();
        if (es == nullptr) {
            LOG(ERROR) << "Can not select valid primary es.";
            return Status(common::CYPRE_EM_RG_ALLOCATE_FAIL);
        }
        rg.second->get_extent_servers().push_back(es->get_id());
    }

    // allocate secondary
    for (int i = 0; i < replica_count - 1; i++) {
        for (auto &rg : rg_map_) {
            auto es = rg_allocater_->select_secondary(
                    rg.second->get_extent_servers());
            if (es == nullptr) {
                LOG(ERROR) << "Can not select valid secondary es.";
                return Status(common::CYPRE_EM_RG_ALLOCATE_FAIL);
            }
            rg.second->get_extent_servers().push_back(es->get_id());
        }
    }

    // persist
    std::vector<std::pair<std::string, std::string>> kvs;
    for (auto &r : rg_map_) {
        std::string value =
                utils::Serializer<ReplicationGroup>::Encode(*r.second.get());
        kvs.push_back(std::make_pair(r.second->kv_key(), value));
        if (kvs.size() == 50) {
            auto status = kv_store_->MultiPut(kvs);
            if (!status.ok()) {
                LOG(ERROR) << "Persist replication group meta to store failed.";
                return Status(
                        common::CYPRE_EM_STORE_ERROR,
                        "persist replication meta failed");
            }
            kvs.clear();
        }
    }

    if (!kvs.empty()) {
        auto status = kv_store_->MultiPut(kvs);
        if (!status.ok()) {
            LOG(ERROR) << "Persist replication group meta to store failed.";
            return Status(
                    common::CYPRE_EM_STORE_ERROR,
                    "persist replication meta failed");
        }
    }
    build_heap();

    return Status();
}

void RgManager::build_heap() {
    std::map<int, EsRgPtr> es_rg_map;
    for (auto &r : rg_map_) {
        auto ess = r.second->get_extent_servers();
        int primary = ess[0];
        if (es_rg_map.find(primary) != es_rg_map.end()) {
            auto es_rg = es_rg_map[primary];
            es_rg->rg_heap.push(r.second);
            es_rg->weight++;
            es_rg->nr_extents += r.second->nr_extents_;
        } else {
            EsRgPtr es_rg = std::make_shared<EsRg>(primary);
            es_rg->rg_heap.push(r.second);
            es_rg->weight++;
            es_rg->nr_extents += r.second->nr_extents_;
            es_rg_map[primary] = es_rg;
            es_rg_heap_.push(es_rg);
        }
    }
}

Status
RgManager::query_rg(const std::string &rg_id, common::pb::ReplicaGroup *rg) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = rg_map_.find(rg_id);
    if (iter == rg_map_.end()) {
        LOG(ERROR) << "Query replica group failed, not found rg: " << rg_id;
        return Status(common::CYPRE_EM_RG_NOT_FOUND, "not found rg");
    }
    *rg = iter->second->ToPbReplicationGroup();
    return Status();
}

std::vector<int> RgManager::query_es_group(const std::string &rg_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto iter = rg_map_.find(rg_id);
    return iter->second->extent_servers_;
}

// here just consider extent nums in each rg, but different rg can contain same
// es while different primary, such as, 1, 3, 5 | 5, 3, 1 | 3, 1, 5, this can
// cause some ess hold more load in some time (guess), but even in totally, can
// be a optimize way.
std::string
RgManager::allocate_rg(const std::vector<std::shared_ptr<ExtentServer>> &ess) {
    auto es_rg = es_rg_heap_.top();
    std::list<EsRgPtr> es_rg_tmp;
    bool found = false;
    std::string result = std::string();
    do {
        std::list<RGPtr> rg_tmp;
        auto rg = es_rg->rg_heap.top();
        do {
            if (nearfull(rg->id_, ess)) {
                rg_tmp.push_back(rg);
                es_rg->rg_heap.pop();
                rg = es_rg->rg_heap.top();
            } else {
                found = true;
                break;
            }
        } while (!es_rg->rg_heap.empty());

        for (auto &l : rg_tmp) {
            es_rg->rg_heap.push(l);
        }
        rg_tmp.clear();
        if (found) {
            rg->nr_extents_++;
            es_rg->rg_heap.pop();
            es_rg->rg_heap.push(rg);
            es_rg->nr_extents++;
            result = rg->id_;
            break;
        } else {
            es_rg_tmp.push_back(es_rg);
            es_rg_heap_.pop();
            es_rg = es_rg_heap_.top();
        }
    } while (!es_rg_heap_.empty());

    for (auto &l : es_rg_tmp) {
        es_rg_heap_.push(l);
    }
    if (found) {
        es_rg_heap_.pop();
        es_rg_heap_.push(es_rg);
    }
    return result;
}

Status RgManager::allocate_rgs(
        int extent_num, const std::vector<std::shared_ptr<ExtentServer>> &ess,
        std::vector<std::string> *rg_ids) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    std::set<std::string> rgs;
    for (int i = 0; i < extent_num; i++) {
        auto id = allocate_rg(ess);
        if (!id.empty()) {
            rgs.insert(id);
            rg_ids->push_back(id);
        } else {
            LOG(ERROR) << "Allocate rgs failed";
            return Status(
                    common::CYPRE_EM_RG_ALLOCATE_FAIL,
                    "resource is not enough.");
        }
    }
    std::map<std::string, std::string> map;

    for (auto &id : rgs) {
        auto iter = rg_map_.find(id);
        std::string key = iter->second->kv_key();
        std::string value = utils::Serializer<ReplicationGroup>::Encode(
                *(iter->second.get()));
        map[key] = value;
    }

    std::vector<std::pair<std::string, std::string>> kvs;
    for (auto &m : map) {
        kvs.push_back(std::make_pair(m.first, m.second));
        if (kvs.size() == 100) {
            auto status = kv_store_->MultiPut(kvs);
            if (!status.ok()) {
                LOG(ERROR) << "Persist replication group meta to store failed.";
                return Status(
                        common::CYPRE_EM_STORE_ERROR,
                        "persist replication meta failed");
            }
            kvs.clear();
        }
    }

    if (!kvs.empty()) {
        auto status = kv_store_->MultiPut(kvs);
        if (!status.ok()) {
            LOG(ERROR) << "Persist replication group meta to store failed.";
            return Status(
                    common::CYPRE_EM_STORE_ERROR,
                    "persist replication meta failed");
        }
    }
    return Status();
}

Status RgManager::delete_all_rg() {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    for (auto &m : rg_map_) {
        auto kv_status = kv_store_->Delete(m.second->kv_key());
        if (!kv_status.ok()) {
            LOG(ERROR) << "Delete replication group meta from store failed. rg "
                          "id: "
                       << m.first;
        }
    }
    rg_map_.clear();
    while (!es_rg_heap_.empty()) {
        es_rg_heap_.pop();
    }
    return Status();
}

uint64_t RgManager::total_nr_extents() {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    uint64_t all_nr = 0;
    for (auto &r : rg_map_) {
        all_nr += r.second->nr_extents_;
    }
    return all_nr;
}

Status RgManager::list_rg(std::vector<common::pb::ReplicaGroup> *rgs) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto &r : rg_map_) {
        rgs->push_back(r.second->ToPbReplicationGroup());
    }
    return Status();
}

bool RgManager::nearfull(
        const std::string &rg_id,
        const std::vector<std::shared_ptr<ExtentServer>> &ess) {
    auto rg = rg_map_.find(rg_id);
    auto vec = rg->second->extent_servers_;
    std::map<int, std::shared_ptr<ExtentServer>> map;
    for (auto &es : ess) {
        map[es->get_id()] = es;
    }
    double ratio = 0.0;
    for (auto &v : vec) {
        auto es = map.find(v);
        double tmp = es->second->get_used() / es->second->get_capacity();
        if (tmp > ratio) {
            ratio = tmp;
        }
    }
    return ratio > common::kRatioNearfull;
}

bool RgManager::recovery_from_store(const std::string &pool_id) {
    std::unique_ptr<kvstore::KVIterator> iter;
    std::stringstream ss(
            common::kRGKvPrefix, std::ios_base::app | std::ios_base::out);
    ss << "_" << pool_id;
    std::string prefix = ss.str();
    auto status = kv_store_->ScanPrefix(prefix, &iter);
    if (!status.ok()) {
        LOG(ERROR) << "Scan replication group meta from store failed, status: "
                   << status.ToString();
        return false;
    }
    std::vector<std::string> tmp;
    while (iter->Valid()) {
        tmp.push_back(iter->value());
        iter->Next();
    }
    for (auto &v : tmp) {
        auto rg = std::make_shared<ReplicationGroup>();
        if (utils::Serializer<ReplicationGroup>::Decode(v, *rg.get())) {
            rg_map_[rg->id_] = rg;
        } else {
            LOG(ERROR) << "Decode replication group meta failed";
            return false;
        }
    }
    build_heap();
    LOG(INFO) << "Recover replication group meta seccedd, include: "
              << rg_map_.size() << " replication groups.";
    return true;
}

}  // namespace extentmanager
}  // namespace cyprestore
