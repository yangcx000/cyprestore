/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_COMMON_CTX_MEM_H
#define CYPRESTORE_COMMON_CTX_MEM_H

#include "cypre_ring.h"
#include "mem_buffer.h"
#include "status.h"

namespace cyprestore {
namespace common {

template <class T> class CtxMemMgr {
public:
    CtxMemMgr(
            std::string name, CypreRing::CypreRingType type, bool use_hugepage,
            uint32_t max_units = 1 << 20)
            : type_(type), use_hugepage_(use_hugepage), max_units_(max_units) {}
    ~CtxMemMgr() = default;

    Status Init();

    Status GetCtxUnitBulk(T **ctx);
    Status GetCtxUnitBurst(T **ctx);
    void PutCtxUnit(T *ctx);

    Status GetCtxUnitsBulk(uint32_t num, T **ctx);
    size_t GetCtxUnitsBurst(uint32_t num, T **ctx);
    void PutCtxUnits(uint32_t num, T **ctx);

    uint32_t used() {
        return mem_buffer_->used();
    }

private:
    std::unique_ptr<MemBuffer<T>> mem_buffer_;
    std::string name_;
    CypreRing::CypreRingType type_;
    bool use_hugepage_;
    uint32_t max_units_;
};

template <class T> Status CtxMemMgr<T>::Init() {
    MemBuffer<T> *mem_buffer =
            new MemBuffer<T>(name_, type_, max_units_, use_hugepage_);
    if (!mem_buffer)
        return Status(common::CYPRE_ER_OUT_OF_MEMORY, "new membuffer failed");
    mem_buffer->init();
    mem_buffer_.reset(mem_buffer);
    return Status();
}

template <class T> Status CtxMemMgr<T>::GetCtxUnitBurst(T **ctx) {
    return mem_buffer_->GetBurst(ctx);
}

template <class T> Status CtxMemMgr<T>::GetCtxUnitBulk(T **ctx) {
    return mem_buffer_->GetBulk(ctx);
}

template <class T> void CtxMemMgr<T>::PutCtxUnit(T *ctx) {
    mem_buffer_->Put(ctx);
}

template <class T>
size_t CtxMemMgr<T>::GetCtxUnitsBurst(uint32_t num, T **ctx) {
    return mem_buffer_->GetsBurst(num, ctx);
}

template <class T> Status CtxMemMgr<T>::GetCtxUnitsBulk(uint32_t num, T **ctx) {
    return mem_buffer_->GetsBulk(num, ctx);
}

template <class T> void CtxMemMgr<T>::PutCtxUnits(uint32_t num, T **ctx) {
    mem_buffer_->Puts(num, ctx);
}

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_CTX_MEM_H
