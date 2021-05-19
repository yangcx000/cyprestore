/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_COMMON_ARENA_H
#define CYPRESTORE_COMMON_ARENA_H

#include <cstdint>
#include <mutex>

#include "butil/macros.h"
#include "status.h"

namespace cyprestore {
namespace common {

struct ArenaOptions {
    ArenaOptions()
            : align_size(0), initial_block_size(0), max_block_size(0),
              lock_free(false), use_hugepage(true) {}
    ArenaOptions(
            uint64_t align_size_, uint64_t initial_block_size_,
            uint64_t max_block_size_, bool lock_free_, bool use_hugepage_)
            : align_size(align_size_), initial_block_size(initial_block_size_),
              max_block_size(max_block_size_), lock_free(lock_free_),
              use_hugepage(use_hugepage_) {}

    uint64_t align_size;          // 内存对齐大小
    uint64_t initial_block_size;  // 分配块初始值
    uint64_t max_block_size;      // 分配块最大值
    bool lock_free;
    bool use_hugepage;
};

class Arena {
public:
    Arena(const ArenaOptions &options);
    ~Arena();

    static Status AllocateDirect(
            uint64_t size, bool use_hugepage, uint32_t num,
            std::vector<void *> *addrs);
    static void ReleaseDirect(void *p, bool use_hugepage);

    // all or nothing
    Status Allocate(uint64_t size, uint32_t num, std::vector<void *> *addrs);
    void Release();
    size_t NumBlocks() const {
        return num_blocks_;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(Arena);
    void reset();
    static void *
    allocate_mem(uint64_t size, uint64_t align_size, bool use_hugepage);
    static void free_mem(void *p, bool use_hugepage);
    void lock() {
        if (!options_.lock_free) {
            lock_.lock();
        }
    }

    void unlock() {
        if (!options_.lock_free) {
            lock_.unlock();
        }
    }

    struct Block {
        Block() : size(0), capacity(0), data(nullptr), next(nullptr) {}
        explicit Block(uint64_t capacity_, void *data_)
                : size(0), capacity(capacity_), data(data_), next(nullptr) {}

        uint64_t left_space() const {
            return capacity - size;
        }

        uint64_t size;
        uint64_t capacity;
        void *data;
        Block *next;
    };

    size_t num_blocks_;
    uint64_t block_size_;
    ArenaOptions options_;
    Block head_;
    Block *cur_block_;
    std::mutex lock_;
};

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_ARENA_H
