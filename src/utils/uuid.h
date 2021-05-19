/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_UTILS_UUID_H_
#define CYPRESTORE_UTILS_UUID_H_

#include "butil/guid.h"

namespace cyprestore {
namespace utils {

class UUID {
public:
    static std::string Generate();
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_UUID_H_
