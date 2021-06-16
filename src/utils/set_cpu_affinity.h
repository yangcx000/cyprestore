/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_UTILS_SET_CPU_AFFINITY_H_
#define CYPRESTORE_UTILS_SET_CPU_AFFINITY_H_

#include <pthread.h>

namespace cyprestore {
namespace utils {

class SetCpuAffinityUtil {
public:
    static bool BindCore(pthread_t pthread, int core_num);
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_SET_CPU_AFFINITY_H_


