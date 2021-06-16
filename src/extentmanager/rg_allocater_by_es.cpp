/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "rg_allocater_by_es.h"

#include <list>

#include "common/error_code.h"
#include "utils/gcd_util.h"

namespace cyprestore {
namespace extentmanager {

Status RGAllocaterByEs::init(
        const std::vector<std::shared_ptr<ExtentServer>> &ess,
        uint64_t extent_size) {
    auto status = host_rack_valid(ess);
    if (!status.ok()) {
        return status;
    }
    std::vector<int> nums;
    for (auto &es : ess) {
        nums.push_back(es->get_capacity() / extent_size);
    }
    int r = utils::GCDUtil::gcd(nums);

    for (auto &es : ess) {
        es->set_weight(es->get_capacity() / extent_size / r);
        ess_.push(es);
    }
    return Status();
}

std::shared_ptr<ExtentServer> RGAllocaterByEs::select_primary() {
    auto es = ess_.top();

    es->incr_pri_rgs();
    es->incr_rgs();

    ess_.pop();

    ess_.push(es);
    return es;
}

std::shared_ptr<ExtentServer>
RGAllocaterByEs::select_secondary(const std::vector<int> &ess) {
    std::list<std::shared_ptr<ExtentServer>> tmp_ess;
    auto es = ess_.top();
    bool find = false;

    do {
        auto it = ess.begin();
        for (; it != ess.end(); it++) {
            if ((*it) == es->get_id()) break;
        }

        if (it != ess.end()) {
            tmp_ess.push_back(es);
            ess_.pop();
            es = ess_.top();
        } else {
            find = true;
            break;
        }
    } while (!ess_.empty());

    if (!find) {
        // Restore heap
        for (auto &e : tmp_ess) {
            ess_.push(e);
        }
        return nullptr;
    }

    es->incr_sec_rgs();
    es->incr_rgs();
    ess_.pop();
    ess_.push(es);

    // Restore heap
    for (auto &e : tmp_ess) {
        ess_.push(e);
    }
    return es;
}

}  // namespace extentmanager
}  // namespace cyprestore
