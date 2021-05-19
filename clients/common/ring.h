/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_CLIENTS_CYPRE_BENCH_RING_H
#define CYPRESTORE_CLIENTS_CYPRE_BENCH_RING_H

#include <atomic>
#include <string>
#include <vector>

#include "common/status.h"

namespace cyprestore {
namespace clients {

using common::Status;

class Ring {
public:
    enum RingType {
        RING_SP_SC = 0,
        RING_MP_SC,
        RING_SP_MC,
        RING_MP_MC,
    };

    Ring(const std::string &name, RingType type, uint32_t size)
            : size_(size), mask_(0), capacity_(0), prod_head_(0), prod_tail_(0),
              cons_head_(0), cons_tail_(0), name_(name), type_(type),
              is_sp_(false), is_sc_(false) {}
    ~Ring() = default;

    Status Init();
    Status Enqueue(void *obj);
    Status Dequeue(void **obj);

    uint32_t Used() {
        uint32_t prod_tail = prod_tail_.load();
        uint32_t cons_tail = cons_tail_.load();
        uint32_t count = (prod_tail - cons_tail) & mask_;
        return count > capacity_ ? capacity_ : count;
    }

    uint32_t Free() {
        return capacity_ - Used();
    }
    bool RingFull() {
        return Free() == 0;
    }
    bool RingEmpty() {
        return Used() == 0;
    }
    uint32_t Size() {
        return size_;
    }
    uint32_t Capacity() {
        return capacity_;
    }

private:
    bool PowerOf2(uint32_t size) {
        return (size & (size - 1)) == 0;
    }

    uint32_t size_;
    uint32_t mask_;
    uint32_t capacity_;

    std::atomic<uint32_t> prod_head_;
    std::atomic<uint32_t> prod_tail_;

    std::atomic<uint32_t> cons_head_;
    std::atomic<uint32_t> cons_tail_;

    std::string name_;
    RingType type_;
    bool is_sp_;
    bool is_sc_;
    std::vector<void *> slots_;
};

}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_CYPRE_BENCH_RING_H
