/*
 * Copyright 2021 JDD authors.
 * @feifei5
 *
 */

#include "io_mem.h"

#include "common/constants.h"

namespace cyprestore {
namespace extentserver {

Status IOMem::Init() {
    ArenaOptions options(
            common::kAlignSize, init_block_size_, max_block_size_,
            type_ == CypreRing::CypreRingType::TYPE_SP_SC, true);
    Arena *arena = new Arena(options);
    if (!arena)
        return Status(common::CYPRE_ER_OUT_OF_MEMORY, "new arena failed");
    arena_.reset(arena);

    max_units_ = kMaxMemSize_ / mem_unit_size_;
    std::string name = "io_mem_" + std::to_string(mem_unit_size_ / 1024) + "K";
    MemBuffer<io_u> *mem_buffer =
            new MemBuffer<io_u>(name, type_, max_units_, true);
    if (!mem_buffer)
        return Status(common::CYPRE_ER_OUT_OF_MEMORY, "new membuffer failed");
    mem_buffer->init();
    mem_buffer_.reset(mem_buffer);

    return Status();
}

Status IOMem::GetIOUnitBulk(io_u **io) {
    auto status = mem_buffer_->GetBulk(io);
    if (!status.ok()) return status;

    if ((*io)->data != nullptr) {
        return Status();
    }

    std::vector<void *> addrs;
    status = arena_->Allocate(mem_unit_size_, 1, &addrs);
    if (!status.ok()) {
        // TODO here can not delete *io, because it may free all mem follow *io,
        // because mem was allocated as a whole
        // so three will be a segment mem not used for ever
        LOG(ERROR) << "allocate mem faild, out of memory";
        mem_buffer_->ring_sub(1);
        *io = nullptr;
        return status;
    }

    (*io)->data = addrs[0];
    (*io)->size = mem_unit_size_;
    return Status();
}

Status IOMem::GetIOUnitBurst(io_u **io) {
    return mem_buffer_->GetBurst(io);
}

Status IOMem::PutIOUnit(io_u *io) {
    return mem_buffer_->Put(io);
}

Status IOMem::GetIOUnitsBulk(uint32_t num, io_u **ios) {
    auto status = mem_buffer_->GetsBulk(num, ios);
    if (!status.ok()) return status;

    // all hava mem
    if (ios[0]->data != nullptr) {
        return Status();
    }

    // allocate new
    std::vector<void *> addrs;
    status = arena_->Allocate(mem_unit_size_, num, &addrs);
    if (!status.ok()) {
        // TODO here can not delete *io, because it may free all mem follow *io,
        // because mem was allocated as a whole
        // so three will be some segment mem not used for ever
        LOG(ERROR) << "allocate mem faild, out of memory";
        mem_buffer_->ring_sub(num);
        for (uint32_t i = 0; i < num; i++) {
            ios[i] = nullptr;
        }
        return status;
    }

    for (uint32_t i = 0; i < num; i++) {
        ios[i]->data = addrs[i];
        ios[i]->size = mem_unit_size_;
    }
    return Status();
}

size_t IOMem::GetIOUnitsBurst(uint32_t num, io_u **ios) {
    return mem_buffer_->GetsBurst(num, ios);
}

Status IOMem::PutIOUnits(uint32_t num, io_u **ios) {
    return mem_buffer_->Puts(num, ios);
}

Status IOMemMgr::Init() {
    uint32_t init_slab = kIOUnitSize_;
    int index = 0;
    io_mems_.resize(max_slab_ / init_slab);
    while (init_slab <= max_slab_) {
        IOMem *io_mem = new IOMem(init_slab, type_);
        if (!io_mem)
            return Status(common::CYPRE_ER_OUT_OF_MEMORY, "out of memory");
        auto status = io_mem->Init();
        if (!status.ok()) return status;
        io_mems_[index].reset(io_mem);
        index++;
        init_slab = kIOUnitSize_ * (index + 1);
    }
    return Status();
}

Status IOMemMgr::GetIOUnitBurst(uint64_t unit_size, io_u **io) {
    uint32_t index = get_index(unit_size);
    if (index < io_mems_.size()) {
        return io_mems_[index]->GetIOUnitBurst(io);
    }

    return Status(
            common::CYPRE_ER_INVALID_ARGUMENT,
            "unit size invalid, no ring for this size");
}

Status IOMemMgr::GetIOUnitBulk(uint64_t unit_size, io_u **io) {
    uint32_t index = get_index(unit_size);
    if (index < io_mems_.size()) {
        return io_mems_[index]->GetIOUnitBulk(io);
    }

    return alloc_direct(unit_size, 1, io);
}

void IOMemMgr::PutIOUnit(io_u *io) {
    uint32_t index = get_index(io->size);
    if (index < io_mems_.size()) {
        auto status = io_mems_[index]->PutIOUnit(io);
        if (!status.ok())
            LOG(ERROR) << "Couldn't put io unit, " << status.ToString();
        return;
    }

    return free_direct(io);
}

Status IOMemMgr::GetIOUnitsBulk(uint64_t unit_size, uint32_t num, io_u **ios) {
    uint32_t index = get_index(unit_size);
    if (index < io_mems_.size()) {
        return io_mems_[index]->GetIOUnitsBulk(num, ios);
    }

    return alloc_direct(unit_size, num, ios);
}

size_t IOMemMgr::GetIOUnitsBurst(uint64_t unit_size, uint32_t num, io_u **ios) {
    uint32_t index = get_index(unit_size);
    if (index < io_mems_.size()) {
        return io_mems_[index]->GetIOUnitsBurst(num, ios);
    }

    return 0;
}

void IOMemMgr::PutIOUnits(uint32_t num, io_u **ios) {
    uint32_t index = get_index(ios[0]->size);
    if (index < io_mems_.size()) {
        auto status = io_mems_[index]->PutIOUnits(num, ios);
        if (!status.ok())
            LOG(ERROR) << "Couldn't put io units, " << status.ToString();
        return;
    }

    return free_direct(num, ios);
}

Status IOMemMgr::alloc_direct(uint64_t unit_size, uint32_t num, io_u **ios) {
    std::vector<void *> addrs;
    auto status = Arena::AllocateDirect(unit_size, true, num, &addrs);
    if (!status.ok()) {
        return status;
    }
    for (uint32_t i = 0; i < num; i++) {
        ios[i] = new io_u(addrs[i], unit_size);
    }
    return Status();
}

// because allocate mem just once, then split into num segments,
// so just free the first addr
// TODO here exists risk if the array order changed
void IOMemMgr::free_direct(uint32_t num, io_u **ios) {
    Arena::ReleaseDirect(ios[0]->data, true);
    for (uint32_t i = 0; i < num; i++) {
        delete ios[i];
    }
}

void IOMemMgr::free_direct(io_u *io) {
    Arena::ReleaseDirect(io->data, true);
    delete io;
}

}  // namespace extentserver
}  // namespace cyprestore
