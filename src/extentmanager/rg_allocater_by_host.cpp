/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "rg_allocater_by_host.h"

#include "common/error_code.h"
#include "utils/gcd_util.h"

namespace cyprestore {
namespace extentmanager {

Status RGAllocaterByHost::init(
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
            hosts_.push(host);
        }

        es_map_[es->get_id()] = es;
    }
    return Status();
}

std::shared_ptr<ExtentServer> RGAllocaterByHost::select_primary() {
    auto host = hosts_.top();
    auto es = host->ess.top();

    host->stat.rgs++;
    host->stat.primary_rgs++;
    es->incr_pri_rgs();
    es->incr_rgs();

    hosts_.pop();
    host->ess.pop();

    hosts_.push(host);
    host->ess.push(es);

    return es;
}

std::shared_ptr<ExtentServer>
RGAllocaterByHost::select_secondary(const std::vector<int> &ess) {
    std::list<HostPtr> tmp_hosts;
    auto host = hosts_.top();
    bool find = false;

    do {
        auto it = ess.begin();
        for (; it != ess.end(); it++) {
            if (es_map_[*it]->get_host() == host->name) break;
        }

        if (it != ess.end()) {
            tmp_hosts.push_back(host);
            hosts_.pop();
            host = hosts_.top();
        } else {
            find = true;
            break;
        }
    } while (!hosts_.empty());

    if (!find) {
        // Restore heap
        for (auto &h : tmp_hosts) {
            hosts_.push(h);
        }
        return nullptr;
    }

    auto es = host->ess.top();

    host->stat.rgs++;
    host->stat.secondary_rgs++;
    es->incr_sec_rgs();
    es->incr_rgs();

    hosts_.pop();
    host->ess.pop();

    hosts_.push(host);
    host->ess.push(es);

    // Restore heap
    for (auto &h : tmp_hosts) {
        hosts_.push(h);
    }

    return es;
}

}  // namespace extentmanager
}  // namespace cyprestore
