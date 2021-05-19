/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "cypre_ring.h"

#include <atomic>
#include <sstream>

#include "butil/logging.h"

namespace cyprestore {
namespace common {

CypreRing::CypreRing(const std::string &name, CypreRingType type, size_t size)
        : name_(name), type_(type), size_(size), impl_(nullptr) {}

CypreRing::~CypreRing() {
    if (impl_) {
        rte_ring_free(impl_);
        impl_ = nullptr;
    }
}

Status CypreRing::Init() {
    unsigned flags = RING_F_EXACT_SZ;
    switch (type_) {
        case TYPE_SP_SC:
            flags |= RING_F_SP_ENQ | RING_F_SC_DEQ;
            break;
        case TYPE_MP_SC:
            flags |= RING_F_SC_DEQ;
            break;
        case TYPE_SP_MC:
            flags |= RING_F_SP_ENQ;
            break;
        case TYPE_MP_MC:
            break;
        default:
            return Status(
                    common::CYPRE_ER_INVALID_ARGUMENT, "invalid ring type");
    }

    static std::atomic<uint32_t> ring_num(0);
    std::stringstream ss;
    ss << name_ << "_" << ring_num.fetch_add(1);
    std::string name = ss.str();
    impl_ = rte_ring_create(name.c_str(), size_, -1, flags);
    if (impl_ == nullptr) {
        return Status(
                common::CYPRE_ES_RTE_RING_CREATE_FAIL,
                "couldn't create rte ring");
    }

    return Status();
}

/* Enqueue one entry */
size_t CypreRing::Enqueue(void **obj) {
    return rte_ring_enqueue_bulk(impl_, obj, 1, NULL);
}

/* Enqueue multi entries */
size_t CypreRing::Enqueue(void **objs, size_t n) {
    return rte_ring_enqueue_bulk(impl_, objs, n, NULL);
}

/* Dequeue one entry */
size_t CypreRing::Dequeue(void **obj) {
    return rte_ring_dequeue_bulk(impl_, obj, 1, NULL);
}

/* Dequeue multi entries, return actual numb of objects dequeued */
size_t CypreRing::DequeueBurst(void **objs, size_t n) {
    return rte_ring_dequeue_burst(impl_, objs, n, NULL);
}

/* Dequeue multi entries, return 0 or n */
size_t CypreRing::DequeueBulk(void **objs, size_t n) {
    return rte_ring_dequeue_bulk(impl_, objs, n, NULL);
}

}  // namespace common
}  // namespace cyprestore
