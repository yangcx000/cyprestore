/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_SPACE_ALLOC_H
#define CYPRESTORE_EXTENTSERVER_SPACE_ALLOC_H

#include <memory>

#include "bitmap_allocator.h"
#include "common/status.h"

namespace cyprestore {
namespace extentserver {

class SpaceAlloc;
typedef std::shared_ptr<SpaceAlloc> SpaceAllocPtr;

using common::Status;

class SpaceAlloc {
public:
    explicit SpaceAlloc(uint64_t disk_capacity)
            : disk_capacity_(disk_capacity) {}
    ~SpaceAlloc() = default;

    int Init();
    Status Allocate(uint64_t want_size, std::unique_ptr<AUnit> *aunit);
    void Free(const std::unique_ptr<AUnit> *aunit);
    void Free(uint64_t offset, uint64_t size);
    void Mark(uint64_t offset, uint64_t size);

    uint64_t BlockSize() const {
        return bitmap_alloc_->BlockSize();
    }
    uint64_t Capacity() const {
        return bitmap_alloc_->Capacity();
    }
    uint64_t UsedSize() const {
        return bitmap_alloc_->UsedSize();
    }

private:
    const uint64_t kAllocateBlockSize = 1 << 20;  // 1MB
    const uint64_t disk_capacity_;
    std::unique_ptr<BitmapAllocator> bitmap_alloc_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_SPACE_ALLOC_H
