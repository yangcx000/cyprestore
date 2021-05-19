/*
 * Copyright 2021 JDD authors.
 * @feifei5
 *
 */

#ifndef CYPRESTORE_COMMON_MEM_BUFFER_H
#define CYPRESTORE_COMMON_MEM_BUFFER_H

#include <bvar/bvar.h>

#include <atomic>
#include <memory>
#include <string>

#include "arena.h"
#include "butil/logging.h"
#include "constants.h"
#include "cypre_ring.h"
#include "status.h"

namespace cyprestore {
namespace common {

template <class T> class MemBuffer {
public:
    MemBuffer(
            std::string name, CypreRing::CypreRingType type, uint32_t size,
            bool use_hugepage)
            : name_(name), type_(type), ring_size_(size),
              use_hugepage_(use_hugepage) {
        num_units_ = 0;
    }
    ~MemBuffer() = default;

    Status init();
    void ring_sub(uint32_t num);
    uint32_t used() {
        return num_units_.load();
    }
    Status GetBurst(T **unit);
    Status GetBulk(T **unit);
    Status Put(T *unit);
    size_t GetsBurst(uint32_t num, T **units);
    Status GetsBulk(uint32_t num, T **units);
    Status Puts(uint32_t num, T **units);

private:
    std::unique_ptr<CypreRing> ring_;
    std::unique_ptr<Arena> arena_;
    std::string name_;  // ring name
    CypreRing::CypreRingType type_;
    uint32_t ring_size_;
    bool use_hugepage_;
    std::atomic<uint32_t> num_units_;
    uint64_t init_block_size_ = 1 << 20;  // 1M
    uint64_t max_block_size_ = 4 << 20;   // 4M

    // bvar::Status<uint32_t> ring_used_;
};

template <class T> Status MemBuffer<T>::init() {
    CypreRing *ring = new CypreRing(name_, type_, ring_size_);
    Status status = ring->Init();
    if (!status.ok()) {
        return status;
    }
    ring_.reset(ring);

    ArenaOptions options(
            kAlignSize, init_block_size_, max_block_size_,
            type_ == CypreRing::CypreRingType::TYPE_SP_SC, use_hugepage_);
    Arena *arena = new Arena(options);
    if (!arena) {
        return Status(common::CYPRE_ER_OUT_OF_MEMORY, "new area failed");
    }
    arena_.reset(arena);
    // ring_used_.set_value(0);
    // ring_used_.expose(name_ + "_ring_used_count");

    return Status();
}

template <class T> Status MemBuffer<T>::GetBulk(T **unit) {
    void *array[1];
    array[0] = (T *)*unit;
    auto re = ring_->Dequeue(array);
    if (re != 0) {
        *unit = (T *)array[0];
        return Status();
    }
    uint32_t ring_used = 0;
    do {
        ring_used = num_units_.load();
        if (ring_used >= ring_size_) {
            LOG(ERROR) << "ring used: " << ring_used;
            return Status(common::CYPRE_ES_RTE_RING_FULL, "ring full");
        }
    } while (!num_units_.compare_exchange_weak(ring_used, ring_used + 1));

    std::vector<void *> addrs;
    auto status = arena_->Allocate(sizeof(T), 1, &addrs);
    if (!status.ok()) {
        num_units_.fetch_sub(1);
        return status;
    }
    *unit = (T *)addrs[0];
    // ring_used_.set_value(num_units_.load());
    return Status();
}

template <class T> Status MemBuffer<T>::GetBurst(T **unit) {
    void *array[1];
    array[0] = (T *)*unit;
    auto re = ring_->Dequeue(array);
    if (re != 0) {
        *unit = (T *)array[0];
        return Status();
    }
    return Status(common::CYPRE_ES_RTE_RING_EMPTY, "ring empty");
}

template <class T> Status MemBuffer<T>::Put(T *unit) {
    void *array[1];
    array[0] = (T *)unit;
    auto re = ring_->Enqueue(array);
    if (re == 0) {
        // TODO handle error, should release unit?
        return Status(common::CYPRE_ES_RTE_RING_FULL, "ring full");
    }
    return Status();
}

template <class T> Status MemBuffer<T>::GetsBulk(uint32_t num, T **units) {
    auto re = ring_->DequeueBulk((void **)units, num);
    if (re != 0) {
        return Status();
    }
    uint32_t ring_used = 0;
    do {
        ring_used = num_units_.load();
        if (ring_used >= ring_size_) {
            return Status(common::CYPRE_ES_RTE_RING_FULL, "ring full");
        }
    } while (!num_units_.compare_exchange_weak(ring_used, ring_used + num));

    std::vector<void *> addrs;
    auto status = arena_->Allocate(sizeof(T), num, &addrs);
    if (!status.ok()) {
        num_units_.fetch_sub(num);
        return status;
    }
    int index = 0;
    for (auto &addr : addrs) {
        units[index] = (T *)addr;
        index++;
    }
    // ring_used_.set_value(num_units_.load());
    return Status();
}

template <class T> size_t MemBuffer<T>::GetsBurst(uint32_t num, T **units) {
    return ring_->DequeueBurst((void **)units, num);
}

template <class T> Status MemBuffer<T>::Puts(uint32_t num, T **units) {
    auto re = ring_->Enqueue((void **)units, num);
    if (re == 0) {
        // TODO handle error, should release
        return Status(common::CYPRE_ES_RTE_RING_FULL, "ring full");
    }
    return Status();
}

template <class T> void MemBuffer<T>::ring_sub(uint32_t num) {
    num_units_.fetch_sub(num);
    // ring_used_.set_value(num_units_.load());
}

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_MEM_BUFFER_H
