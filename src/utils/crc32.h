/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
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
