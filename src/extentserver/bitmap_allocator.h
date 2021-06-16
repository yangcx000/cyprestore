/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTSERVER_BITMAP_ALLOCATOR_H_
#define CYPRESTORE_EXTENTSERVER_BITMAP_ALLOCATOR_H_

#include <memory>
#include <mutex>
#include <vector>

#include "butil/macros.h"
#include "common/status.h"

namespace cyprestore {
namespace extentserver {

struct AUnit;
typedef std::shared_ptr<AUnit> AUnitPtr;

const uint64_t kBlocksPerByte = 8;
const uint64_t kBytesPerUnit = sizeof(uint64_t);                 // 8
const uint64_t kBlocksPerUnit = kBlocksPerByte * kBytesPerUnit;  // 8 * 8 = 64
const uint64_t kMaxBlocksPerChunk = 1024 * 1024;

struct AUnit {
    AUnit() : offset(0), size(0) {}
    AUnit(uint64_t offset_, uint64_t size_) : offset(offset_), size(size_) {}

    void Clear() {
        offset = 0;
        size = 0;
    }

    uint64_t offset;
    uint64_t size;
};

using common::Status;

class Chunk {
public:
    Chunk(uint64_t chunk_idx, uint64_t num_blocks, uint64_t block_size)
            : chunk_idx_(chunk_idx), num_blocks_(num_blocks),
              num_free_blocks_(num_blocks), last_free_bit_(0),
              block_size_(block_size) {
        data_.resize((num_blocks + kBlocksPerUnit - 1) / kBlocksPerUnit, 0);
    }

    Status Allocate(uint64_t num_blocks, AUnit *aunit);
    void Free(uint64_t offset, uint64_t size);
    void Mark(uint64_t offset, uint64_t size);

    uint64_t UsedBlocks() {
        return num_blocks_ - FreeBlocks();
    }
    uint64_t FreeBlocks() {
        return num_free_blocks_;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(Chunk);
    void setBlock(uint64_t begin, uint64_t end);
    void unsetBlock(uint64_t begin, uint64_t end);
    bool avaliable(uint64_t num_blocks) {
        return num_free_blocks_ >= num_blocks;
    }
    uint64_t globalBlockIndex() {
        return chunk_idx_ * kMaxBlocksPerChunk;
    }
    uint64_t getIndex(uint64_t begin, uint64_t count) {
        return (begin + count) / kBlocksPerUnit;
    }
    uint64_t getOffset(uint64_t begin, uint64_t count) {
        return (begin + count) % kBlocksPerUnit;
    }

    uint64_t chunk_idx_;
    uint64_t num_blocks_;
    uint64_t num_free_blocks_;
    uint64_t last_free_bit_;
    uint64_t block_size_;
    std::mutex lock_;
    std::vector<uint64_t> data_;
};

class BitmapAllocator {
public:
    BitmapAllocator() = default;
    ~BitmapAllocator() = default;

    Status Init(uint64_t block_size, uint64_t capacity);
    Status Allocate(uint64_t want_size, AUnit *aunit);
    void Free(uint64_t offset, uint64_t size);
    void Mark(uint64_t offset, uint64_t size);

    uint64_t NumBlocks() {
        return capacity_ / block_size_;
    }
    uint64_t NumChunks() {
        return (NumBlocks() + kMaxBlocksPerChunk - 1) / kMaxBlocksPerChunk;
    }

    bool Empty() {
        return NumChunks() == 0;
    }

    uint64_t UsedBlocks() {
        uint64_t used_blocks = 0;
        uint64_t num_chunks = NumChunks();
        for (uint64_t i = 0; i < num_chunks; ++i) {
            used_blocks += chunks_[i]->UsedBlocks();
        }
        return used_blocks;
    }

    uint64_t UsedSize() {
        return UsedBlocks() * block_size_;
    }

    uint64_t FreeBlocks() {
        uint64_t free_blocks = 0;
        uint64_t num_chunks = NumChunks();
        for (uint64_t i = 0; i < num_chunks; ++i) {
            free_blocks += chunks_[i]->FreeBlocks();
        }
        return free_blocks;
    }

    uint64_t BlockSize() const {
        return block_size_;
    }
    uint64_t Capacity() const {
        return capacity_;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(BitmapAllocator);
    uint64_t calcBlocksBySize(uint64_t size) {
        return (size + block_size_ - 1) / block_size_;
    }
    uint64_t calcChunkIdxByOffset(uint64_t offset) {
        uint64_t block_index = offset / block_size_;
        uint64_t chunk_index = block_index / kMaxBlocksPerChunk;
        return chunk_index;
    }

    uint64_t block_size_;
    uint64_t capacity_;
    std::vector<std::unique_ptr<Chunk>> chunks_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_BITMAP_ALLOCATOR_H_
