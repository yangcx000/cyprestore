/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "arena.h"

#include <algorithm>
#include <cstdlib>

#include "butil/logging.h"
#include "constants.h"
#include "spdk/env.h"

namespace cyprestore {
namespace common {

Arena::Arena(const ArenaOptions &options)
        : num_blocks_(0), block_size_(options.initial_block_size),
          options_(options), cur_block_(nullptr) {}

Arena::~Arena() {
    if (head_.next != nullptr) Release();
}

void *
Arena::allocate_mem(uint64_t size, uint64_t align_size, bool use_hugepage) {
    if (use_hugepage) {
        return spdk_dma_zmalloc(size, align_size, nullptr);
    }
    return aligned_alloc(size, align_size);
}

void Arena::free_mem(void *p, bool use_hugepage) {
    if (use_hugepage) {
        spdk_dma_free(p);
    } else {
        free(p);
    }
}

Status Arena::AllocateDirect(
        uint64_t size, bool use_hugepage, uint32_t num,
        std::vector<void *> *addrs) {
    void *mem = allocate_mem(size * num, kAlignSize, use_hugepage);
    if (!mem) {
        LOG(ERROR) << "Couldn't alloc mem"
                   << ", unit_size: " << size << ", num: " << num;
        return Status(common::CYPRE_ER_OUT_OF_MEMORY, "alloc mem fail");
    }
    for (uint32_t i = 0; i < num; i++) {
        void *res = (char *)mem + i * size;
        addrs->push_back(res);
    }
    return Status();
}

void Arena::ReleaseDirect(void *p, bool use_hugepage) {
    free_mem(p, use_hugepage);
}

// all or nothing
Status
Arena::Allocate(uint64_t size, uint32_t num, std::vector<void *> *addrs) {
    lock();
    void *old_data = nullptr;
    uint64_t old_size = 0;

    if (cur_block_ != nullptr) {
        old_data = cur_block_->data;
        old_size = cur_block_->size;

        while (cur_block_->left_space() >= size && num > 0) {
            void *res = (char *)cur_block_->data + cur_block_->size;
            cur_block_->size += size;
            addrs->push_back(res);
            num--;
        }
        if (num == 0) {
            unlock();
            return Status();
        }
    }

    uint64_t alloc_size = std::min(block_size_, options_.max_block_size);
    // allcote needed mem at least
    alloc_size = std::max(alloc_size, num * size);
    void *mem = allocate_mem(
            alloc_size, options_.align_size, options_.use_hugepage);
    if (!mem) {
        LOG(ERROR) << "Couldn't alloc mem"
                   << ", block_size:" << block_size_
                   << ", alloc_size:" << alloc_size
                   << ", align_size:" << options_.align_size;
        // if allocate failed, recovery old mem
        if (cur_block_ != nullptr) {
            cur_block_->data = old_data;
            cur_block_->size = old_size;
            addrs->clear();
        }
        unlock();
        return Status(common::CYPRE_ER_OUT_OF_MEMORY, "alloc mem fail");
    }

    Block *new_block = new Block(alloc_size, mem);

    while (num > 0) {
        void *res = (char *)new_block->data + new_block->size;
        new_block->size += size;
        addrs->push_back(res);
        num--;
    }

    if (cur_block_ == nullptr) {
        head_.next = new_block;
    } else {
        cur_block_->next = new_block;
        // TODO modify factor
        if (block_size_ < options_.max_block_size) block_size_ <<= 1;
    }

    cur_block_ = new_block;
    ++num_blocks_;
    unlock();
    return Status();
}

void Arena::Release() {
    lock();
    Block *p = head_.next, *tmp = nullptr;
    while (p) {
        free_mem(p->data, options_.use_hugepage);
        tmp = p;
        p = p->next;
        delete tmp;
    }
    reset();
    unlock();
}

void Arena::reset() {
    num_blocks_ = 0;
    block_size_ = options_.initial_block_size;
    head_.next = nullptr;
    cur_block_ = nullptr;
}

}  // namespace common
}  // namespace cyprestore
