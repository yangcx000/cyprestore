/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_UTILS_GCDUTIL_H_
#define CYPRESTORE_UTILS_GCDUTIL_H_

#include <vector>

namespace cyprestore {
namespace utils {

class GCDUtil {
public:
    static int gcd(const std::vector<int> &nums);
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_GCDUTIL_H_
