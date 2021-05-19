/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "utils/tools.h"

#include <limits.h>
#include <unistd.h>

#include <cassert>

namespace cyprestore {
namespace utils {

std::string GetHostName() {
    char hostname[HOST_NAME_MAX + 1];
    gethostname(hostname, HOST_NAME_MAX + 1);
    return std::string(hostname);
}

}  // namespace utils
}  // namespace cyprestore
