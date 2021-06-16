/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "connection_pool.h"

#include <brpc/channel.h>

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

namespace cyprestore {
namespace common {

// thread_local std::unordered_map<std::string, ConnectionPtr>
// ConnectionPool::connection_pool_;

}  // namespace common
}  // namespace cyprestore
