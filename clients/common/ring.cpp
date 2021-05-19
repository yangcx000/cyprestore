/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "ring.h"

#include "common/error_code.h"

namespace cyprestore {
namespace clients {

static inline void pause() {}

Status Ring::Init() {
    if (!PowerOf2(size_)) {
        return Status(
                common::CYPRE_ER_INVALID_ARGUMENT, "size must be power of 2");
    }
    mask_ = size_ - 1;
    capacity_ = mask_;
    prod_head_ = prod_tail_ = 0;
    cons_head_ = cons_tail_ = 0;
    if (type_ == RING_SP_SC || type_ == RING_SP_MC) {
        is_sp_ = true;
    }

    if (type_ == RING_SP_SC || type_ == RING_MP_SC) {
        is_sc_ = true;
    }
    slots_.resize(size_);
    return Status();
}

Status Ring::Enqueue(void *obj) {
    uint32_t prod_head = 0, prod_next = 0;
    bool success = false;

    do {
        prod_head = prod_head_.load(std::memory_order_relaxed);
        if (capacity_ + cons_tail_.load(std::memory_order_acquire) - prod_head
            <= 0) {
            return Status(common::CYPRE_C_RING_FULL, "ring has full");
        }
        prod_next = prod_head + 1;
        if (is_sp_) {
            prod_head_.store(prod_next, std::memory_order_relaxed);
            success = true;
        } else {
            success = prod_head_.compare_exchange_weak(
                    prod_head, prod_next, std::memory_order_relaxed);
        }
    } while (!success);

    slots_[prod_head & mask_] = obj;
    if (!is_sp_) {
        while (prod_tail_.load(std::memory_order_relaxed) != prod_head) pause();
    }
    prod_tail_.store(prod_next, std::memory_order_release);
    return Status();
}

Status Ring::Dequeue(void **obj) {
    uint32_t cons_head = 0, cons_next = 0;
    bool success = false;

    do {
        cons_head = cons_head_.load(std::memory_order_relaxed);
        if (prod_tail_.load(std::memory_order_acquire) - cons_head <= 0) {
            return Status(common::CYPRE_C_RING_EMPTY, "ring empty");
        }
        cons_next = cons_head + 1;
        if (is_sc_) {
            cons_head_.store(cons_next, std::memory_order_relaxed);
            success = true;
        } else {
            success = cons_head_.compare_exchange_weak(
                    cons_head, cons_next, std::memory_order_relaxed);
        }
    } while (!success);

    *obj = slots_[cons_head & mask_];
    if (!is_sc_) {
        while (cons_tail_.load(std::memory_order_relaxed) != cons_head) pause();
    }
    cons_tail_.store(cons_next, std::memory_order_release);
    return Status();
}

}  // namespace clients
}  // namespace cyprestore
