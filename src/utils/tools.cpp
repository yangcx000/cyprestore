/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
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
