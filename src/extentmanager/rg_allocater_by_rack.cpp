/*
 * Copyright 2020 JDD authors
 * @feifei5
 *
 */

#include "rg_allocater_by_rack.h"

#include "common/error_code.h"
#include "utils/gcd_util.h"

namespace cyprestore {
namespace extentmanager {

Status RGAllocaterByRack::init(
        const std::vector<std::shared_ptr<ExtentServer>> &ess,
        uint64_t extent_size) {
    auto status = host_rack_valid(ess);
    if (!status.ok()) {
        return status;
    }

    // greatest common divisor
    std::vector<int> nums;
    for (auto &es : ess) {
        nums.push_back(es->get_capacity() / extent_size);
    }
    int r = utils::GCDUtil::gcd(nums);

    std::unordered_map<std::string, HostPtr> hosts;
    std::unordered_map<std::string, RackPtr> racks;
    for (auto &es : ess) {
        es->set_weight(es->get_capacity() / extent_size / r);
        if (hosts.find(es->get_host()) != hosts.end()) {
            auto host = hosts[es->get_host()];
            host->ess.push(es);
            host->stat.weight += es->get_weight();
        } else {
            HostPtr host = std::make_shared<Host>(es->get_host());
            host->ess.push(es);
            host->stat.weight += es->get_weight();
            hosts[es->get_host()] = host;
        }

        if (racks.find(es->get_rack()) != racks.end()) {
            auto rack = racks[es->get_rack()];
            rack->hosts.push(hosts[es->get_host()]);
            rack->stat.weight += es->get_weight();
        } else {
            RackPtr rack = std::make_shared<Rack>(es->get_rack());
            rack->hosts.push(hosts[es->get_host()]);
            rack->stat.weight += es->get_weight();
            racks[es->get_rack()] = rack;
            racks_.push(rack);
        }

        es_map_[es->get_id()] = es;
    }
    return Status();
}

std::shared_ptr<ExtentServer> RGAllocaterByRack::select_primary() {
    auto rack = racks_.top();
    auto host = rack->hosts.top();
    auto es = host->ess.top();

    rack->stat.rgs++;
    rack->stat.primary_rgs++;
    host->stat.rgs++;
    host->stat.primary_rgs++;
    es->incr_pri_rgs();
    es->incr_rgs();

    racks_.pop();
    rack->hosts.pop();
    host->ess.pop();

    racks_.push(rack);
    rack->hosts.push(host);
    host->ess.push(es);
    return es;
}

std::shared_ptr<ExtentServer>
RGAllocaterByRack::select_secondary(const std::vector<int> &ess) {
    RackList tmp_racks;
    auto rack = racks_.top();
    bool find = false;

    do {
        auto it = ess.begin();
        for (; it != ess.end(); it++) {
            if (es_map_[*it]->get_rack() == rack->name) break;
        }

        if (it != ess.end()) {
            tmp_racks.push_back(rack);
            racks_.pop();
            rack = racks_.top();
        } else {
            find = true;
            break;
        }
    } while (!racks_.empty());

    if (!find) {
        // Restore heap
        for (auto &r : tmp_racks) {
            racks_.push(r);
        }
        return nullptr;
    }

    auto host = rack->hosts.top();
    auto es = host->ess.top();

    rack->stat.rgs++;
    rack->stat.secondary_rgs++;
    host->stat.rgs++;
    host->stat.secondary_rgs++;
    es->incr_sec_rgs();
    es->incr_rgs();

    racks_.pop();
    rack->hosts.pop();
    host->ess.pop();

    racks_.push(rack);
    rack->hosts.push(host);
    host->ess.push(es);

    // Restore heap
    for (auto &r : tmp_racks) {
        racks_.push(r);
    }
    return es;
}

}  // namespace extentmanager
}  // namespace cyprestore
