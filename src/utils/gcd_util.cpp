/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
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
