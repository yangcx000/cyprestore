/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "set_cpu_affinity.h"

namespace cyprestore {
namespace utils {

bool SetCpuAffinityUtil::BindCore(pthread_t pthread, int core_num) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(core_num, &mask);
    if (CPU_COUNT(&mask) > 0) {
        return pthread_setaffinity_np(pthread, sizeof(mask), &mask) == 0;
    }
    return false;
}

}  // namespace utils
}  // namespace cyprestore`
