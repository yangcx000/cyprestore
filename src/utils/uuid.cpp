/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "uuid.h"

namespace cyprestore {
namespace utils {

std::string UUID::Generate() {
    return butil::GenerateGUID();
}

}  // namespace utils
}  // namespace cyprestore
