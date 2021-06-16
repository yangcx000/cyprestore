/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_COMMON_CYPRE_RING_H_
#define CYPRESTORE_COMMON_CYPRE_RING_H_

#include <string>

#include "rte_ring.h"
#include "status.h"

namespace cyprestore {
namespace common {

class CypreRing {
public:
    enum CypreRingType {
        TYPE_SP_SC = 0,
        TYPE_MP_SC,
        TYPE_SP_MC,
        TYPE_MP_MC,
    };

    CypreRing(const std::string &name, CypreRingType type, size_t size);
    ~CypreRing();

    Status Init();

    /* Enqueue one entry */
    size_t Enqueue(void **obj);

    /* Enqueue multi entries */
    size_t Enqueue(void **objs, size_t n);

    /* Dequeue one entry */
    size_t Dequeue(void **obj);

    /* Dequeue multi entries, return 0 or n*/
    size_t DequeueBulk(void **objs, size_t n);

    /* Dequeue multi entries, return actual number of objects dequeued*/
    size_t DequeueBurst(void **objs, size_t n);

    // void Reset() {
    //  rte_ring_reset(impl_);
    //}

    size_t Size() {
        return rte_ring_get_size(impl_);
    }

    size_t Used() {
        return rte_ring_count(impl_);
    }

    size_t Free() {
        return rte_ring_free_count(impl_);
    }

    bool Empty() {
        return rte_ring_empty(impl_) == 1;
    }

    bool Full() {
        return rte_ring_full(impl_) == 1;
    }

private:
    std::string name_;
    CypreRingType type_;
    size_t size_;
    struct rte_ring *impl_;
};

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_CYPRE_RING_H_
