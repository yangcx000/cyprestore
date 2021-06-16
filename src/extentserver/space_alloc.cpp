/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "space_alloc.h"

#include "butil/logging.h"
#include "common/status.h"
#include "extentserver.h"

namespace cyprestore {
namespace extentserver {

using common::Status;

int SpaceAlloc::Init() {
    BitmapAllocator *alloc = new BitmapAllocator();
    if (!alloc) {
        LOG(ERROR) << "Couldn't new BitmapAllocator";
        return -1;
    }
    Status s = alloc->Init(kAllocateBlockSize, disk_capacity_);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't init BitmapAllocator, " << s.ToString();
        return -1;
    }
    bitmap_alloc_.reset(alloc);
    return 0;
}

Status SpaceAlloc::Allocate(uint64_t want_size, std::unique_ptr<AUnit> *aunit) {
    Status s = bitmap_alloc_->Allocate(want_size, aunit->get());
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't allocate space, " << s.ToString();
        return s;
    }
    return Status();
}

void SpaceAlloc::Free(const std::unique_ptr<AUnit> *aunit) {
    bitmap_alloc_->Free(aunit->get()->offset, aunit->get()->size);
}

void SpaceAlloc::Free(uint64_t offset, uint64_t size) {
    bitmap_alloc_->Free(offset, size);
}

void SpaceAlloc::Mark(uint64_t offset, uint64_t size) {
    bitmap_alloc_->Mark(offset, size);
}

}  // namespace extentserver
}  // namespace cyprestore
