/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "utils/crc32.h"

namespace cyprestore {
namespace utils {

uint32_t Crc32::Checksum(const std::string &data) {
    return butil::crc32c::Value(data.c_str(), data.size());
}

uint32_t Crc32::Checksum(const void *data, uint64_t size) {
    return butil::crc32c::Value((char *)data, size);
}

uint32_t Crc32::Checksum(const butil::IOBuf &buf) {
    butil::IOBufAsZeroCopyInputStream input(buf);
    const void *data = nullptr;
    int size = 0;
    uint32_t crc32 = 0;
    while (input.Next(&data, &size)) {
        crc32 = butil::crc32c::Extend(crc32, (char*)data, size);
    }
    return crc32;
}

}  // namespace utils
}  // namespace cyprestore
