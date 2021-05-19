/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_COMMON_CONSTANTS_H_
#define CYPRESTORE_COMMON_CONSTANTS_H_

#include <string>

namespace cyprestore {
namespace common {

const std::string kUserIdPrefix = "user-";
// pool_id = kPoolIdPrefix + [a-z];
const std::string kPoolIdPrefix = "pool-";
const std::string kRgIdPrefix = "rg-";
const std::string kBlobIdPrefix = "bb-";

const std::string kPoolKvPrefix = "0x00";
const std::string kUserKvPrefix = "0x01";
const std::string kESKvPrefix = "0x02";
const std::string kERKvPrefix = "0x03";
const std::string kRGKvPrefix = "0x04";
const std::string kBlobKvPrefix = "0x05";

// Blob && Extent && Block
const uint64_t kMaxBlobSize = 32 * (1ULL << 40);     // 32TB
const uint32_t kDefaultExtentSize = 1 * (1U << 30);  // 1GB
const uint32_t kDefaultBlockSize = 4 * (1U << 10);   // 4KB
const uint64_t kAlignSize = 4 << 10;                 // 4k

// Bitmap Allocator
const uint32_t kBitmapChunkSize = 64 * (1U << 10);  // 64KB Bitmap chunk size
const uint32_t kBitmapBlockSize = kDefaultExtentSize;

const int kMaxReplicaCount = 3;
const int kRgAvg = 50;
const double kRatioNearfull = 0.85;

const uint32_t kSpdkBDevAlignSize = 4096;

enum Module {
    kAccess = 0,
    kSetManager,
    kExtentManager,
    kExtentServer,
};

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_CONSTANTS_H_
