/*
 * Copyright 2021 JDD authors.
 * @feifei5
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_IO_MEM_H
#define CYPRESTORE_EXTENTSERVER_IO_MEM_H

#include <atomic>

#include "common/arena.h"
#include "common/cypre_ring.h"
#include "common/mem_buffer.h"
#include "common/status.h"

namespace cyprestore {
namespace extentserver {

using common::Arena;
using common::ArenaOptions;
using common::CypreRing;
using common::MemBuffer;
using common::Status;

struct io_u {
    io_u() : data(nullptr), size(0) {}
    io_u(void *data_, size_t size_) : data(data_), size(size_) {}

    void *data;
    size_t size;
};

class IOMem {
public:
    explicit IOMem(uint32_t mem_unit_size, CypreRing::CypreRingType type)
            : mem_unit_size_(mem_unit_size), max_units_(0), type_(type) {}
    ~IOMem() = default;

    Status Init();

    // single api
    Status GetIOUnitBurst(io_u **io);
    Status GetIOUnitBulk(io_u **io);
    Status PutIOUnit(io_u *io);

    // multi api
    Status GetIOUnitsBulk(uint32_t num, io_u **ios);
    size_t GetIOUnitsBurst(uint32_t num, io_u **ios);
    Status PutIOUnits(uint32_t num, io_u **ios);

    uint32_t used() {
        return mem_buffer_->used();
    }

private:
    const uint32_t kMaxMemSize_ = 1 << 31;  // 2 GB
    std::unique_ptr<MemBuffer<io_u>> mem_buffer_;
    std::unique_ptr<Arena> arena_;
    uint32_t mem_unit_size_;
    uint32_t max_units_;
    CypreRing::CypreRingType type_;
    uint32_t init_block_size_ = 4 << 20;
    uint32_t max_block_size_ = 8 << 20;
};

class IOMemMgr {
public:
    explicit IOMemMgr(
            CypreRing::CypreRingType type, uint32_t max_slab = 1 << 20)
            : max_slab_(max_slab), type_(type) {}
    ~IOMemMgr() = default;

    Status Init();
    // signle api
    Status GetIOUnitBurst(uint64_t unit_size, io_u **io);
    Status GetIOUnitBulk(uint64_t unit_size, io_u **io);
    void PutIOUnit(io_u *io);

    // multi api
    size_t GetIOUnitsBurst(uint64_t unit_size, uint32_t num, io_u **ios);
    Status GetIOUnitsBulk(uint64_t unit_size, uint32_t num, io_u **ios);
    void PutIOUnits(uint32_t num, io_u **ios);

private:
    uint32_t get_index(uint64_t unit_size) {
        return (unit_size / kIOUnitSize_) - 1;
    }

    Status alloc_direct(uint64_t unit_size, uint32_t num, io_u **ios);
    void free_direct(io_u *ios);
    void free_direct(uint32_t num, io_u **ios);

    // 4k
    const uint32_t kIOUnitSize_ = 4 << 10;
    uint32_t max_slab_;
    CypreRing::CypreRingType type_;
    std::vector<std::unique_ptr<IOMem>> io_mems_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_IO_MEME_H
