/*
 * Copyright 2021 JDD authors
 * @feifei5
 *
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


