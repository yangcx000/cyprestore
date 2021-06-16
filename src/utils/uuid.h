/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
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
