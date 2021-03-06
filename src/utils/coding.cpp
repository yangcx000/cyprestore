/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "utils/coding.h"

#include <cstring>
#include <string>

namespace cyprestore {
namespace utils {

void Coding::PutFixed32(std::string *dst, uint32_t value) {
    dst->append(
            const_cast<const char *>(reinterpret_cast<char *>(&value)),
            sizeof(value));
}

bool Coding::GetFixed32(std::string *input, uint32_t *value) {
    int left = input->size() - sizeof(uint32_t);
    if (left < 0) {
        return false;
    }
    *value = DecodeFixed32(input->data() + left);
    input->assign(*input, 0, left);
    return true;
}

uint32_t Coding::DecodeFixed32(const char *ptr) {
    uint32_t result;
    memcpy(&result, ptr, sizeof(result));
    return result;
}

}  // namespace utils
}  // namespace cyprestore
