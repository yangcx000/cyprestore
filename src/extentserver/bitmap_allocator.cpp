/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "bitmap_allocator.h"

#include <butil/fast_rand.h>
#include <butil/logging.h>

#include <cassert>

#include "common/constants.h"

namespace cyprestore {
namespace extentserver {

Status Chunk::Allocate(uint64_t num_blocks, AUnit *aunit) {
    std::lock_guard<std::mutex> lock(lock_);
    if (!avaliable(num_blocks))
        return Status(common::CYPRE_ES_DISK_NO_SPACE, "no enough free blocks");

    uint64_t begin = last_free_bit_;
    uint64_t end = num_blocks_ - num_blocks;
    while (begin <= end) {
        uint64_t count = 0, index = 0, offset = 0;
        while (count < num_blocks) {
            index = getIndex(begin, count);
            offset = getOffset(begin, count);
            uint64_t val = data_[index] & (1ULL << offset);
            if (val != 0) {
                begin += num_blocks;
                break;
            }
            ++count;
        }
        if (count == num_blocks) {
            aunit->offset = (globalBlockIndex() + begin) * block_size_;
            aunit->size = num_blocks * block_size_;
            setBlock(begin, begin + num_blocks);
            return Status();
        }
    }

    return Status(common::CYPRE_ES_DISK_NO_SPACE, "no enough space");
}

void Chunk::Mark(uint64_t offset, uint64_t size) {
    uint64_t block_idx = offset / block_size_;
    uint64_t begin = block_idx - globalBlockIndex();
    uint64_t end = begin + size / block_size_;
    setBlock(begin, end);
}

void Chunk::Free(uint64_t offset, uint64_t size) {
    std::lock_guard<std::mutex> lock(lock_);
    uint64_t block_idx = offset / block_size_;
    uint64_t begin = block_idx - globalBlockIndex();
    uint64_t end = begin + size / block_size_;
    unsetBlock(begin, end);
}

void Chunk::setBlock(uint64_t begin, uint64_t end) {
    uint64_t index = 0, offset = 0;
    uint64_t count = end - begin;
    for (uint64_t i = 0; i < count; ++i) {
        index = getIndex(begin, i);
        offset = getOffset(begin, i);
        assert((data_[index] & (1ULL << offset)) == 0 && "bit not equals 0");
        data_[index] |= (1ULL << offset);
    }

    num_free_blocks_ -= count;
    last_free_bit_ = end;
}

void Chunk::unsetBlock(uint64_t begin, uint64_t end) {
    uint64_t index = 0, offset = 0;
    uint64_t count = end - begin;
    for (uint64_t i = 0; i < count; ++i) {
        index = getIndex(begin, i);
        offset = getOffset(begin, i);
        assert((data_[index] & (1ULL << offset)) == 1 && "bit not equals 1");
        data_[index] &= ~(1ULL << offset);
    }

    num_free_blocks_ += count;
    if (begin < last_free_bit_) last_free_bit_ = begin;
}

Status BitmapAllocator::Init(uint64_t block_size, uint64_t capacity) {
    if (block_size == 0 || block_size % common::kAlignSize != 0
        || block_size > capacity) {
        return Status(
                common::CYPRE_ER_INVALID_ARGUMENT,
                "Invalid block_size or capacity");
    }

    block_size_ = block_size;
    capacity_ = capacity;
    uint64_t num_blocks = NumBlocks();
    uint64_t num_chunks = NumChunks();
    chunks_.resize(num_chunks);

    LOG(INFO) << "capacity: " << capacity_ << ", block size: " << block_size_
              << ", block num: " << num_blocks << ", chunk num: " << num_chunks;
    uint64_t blocks_per_chunk = 0;
    for (uint64_t i = 0; i < num_chunks; ++i) {
        blocks_per_chunk = num_blocks > kMaxBlocksPerChunk ? kMaxBlocksPerChunk
                                                           : num_blocks;
        chunks_[i].reset(new Chunk(i, blocks_per_chunk, block_size_));
        num_blocks -= blocks_per_chunk;
    }

    return Status();
}

Status BitmapAllocator::Allocate(uint64_t want_size, AUnit *aunit) {
    if (Empty())
        return Status(common::CYPRE_ES_DISK_EMPTY, "bitmap allocator empty");

    uint64_t num_blocks = calcBlocksBySize(want_size);
    if (num_blocks == 0)
        return Status(common::CYPRE_ER_INVALID_ARGUMENT, "invalid want_size");

    Status s;
    uint64_t num_chunks = chunks_.size();
    uint64_t begin = butil::fast_rand() % num_chunks;
    uint64_t end = begin + num_chunks;
    for (; begin < end; ++begin) {
        uint64_t index = begin % num_chunks;
        s = chunks_[index]->Allocate(num_blocks, aunit);
        if (s.ok()) return s;
    }

    return s;
}

void BitmapAllocator::Free(uint64_t offset, uint64_t size) {
    uint64_t chunk_index = calcChunkIdxByOffset(offset);
    chunks_[chunk_index]->Free(offset, size);
}

void BitmapAllocator::Mark(uint64_t offset, uint64_t size) {
    uint64_t chunk_index = calcChunkIdxByOffset(offset);
    chunks_[chunk_index]->Mark(offset, size);
}

}  // namespace extentserver
}  // namespace cyprestore
