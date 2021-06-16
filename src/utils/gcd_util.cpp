/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "gcd_util.h"

#include <algorithm>

namespace cyprestore {
namespace utils {

int GCDUtil::gcd(const std::vector<int> &nums) {
    int r = nums[0];
    for (auto &num : nums) {
        r = std::__gcd(r, num);
    }
    return r;
}

}  // namespace utils
}  // namespace cyprestore
