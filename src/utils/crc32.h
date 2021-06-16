/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_UTILS_CRC32_H_
#define CYPRESTORE_UTILS_CRC32_H_

#include <string>

#include "butil/crc32c.h"
#include "butil/iobuf.h"

namespace cyprestore {
namespace utils {

class Crc32 {
public:
    static uint32_t Checksum(const std::string &data);
    static uint32_t Checksum(const void *data, uint64_t size);
    static uint32_t Checksum(const butil::IOBuf &buf);
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_CRC32_H_
