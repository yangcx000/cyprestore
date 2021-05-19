/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "io_engine.h"

#include <algorithm>
#include <butil/fast_rand.h>
#include <cassert>
#include <cstring>
#include <memory>

namespace cyprestore {
namespace clients {

IOEngine::IOEngine(const CyprebenchOptions &options_, const std::string &rw_)
        : options(options_), offset(0), io_u_pool(nullptr), rw(rw_) {
    num_blocks = options_.size / options_.block_size;
	// 用于随机读写造IO
    num_align_blocks = (options_.size - options_.block_size) / kAlignSize + 1;
    io_u_pool = new Ring("io_unit", Ring::RING_MP_SC, 1 << 20);
    auto s = io_u_pool->Init();
    assert(s.ok());
}

IOEngine::~IOEngine() {
    delete io_u_pool;
}

IOUnit *IOEngine::GetIO() {
	uint32_t data_len = options.block_size;
    if (options.rw_mode == "seq") {
		if (offset == options.size) {
			offset = 0;	
		} else if ((offset + data_len) > options.size) {
			data_len = options.size - offset;
		}
    } else {
        offset = (butil::fast_rand() % num_align_blocks) * kAlignSize;
    }

    IOUnit *io_u;
    auto s = io_u_pool->Dequeue((void **)&io_u);
    if (!s.ok()) {
        io_u = new IOUnit(offset, options.block_size);
    } else {
        io_u->offset = offset;
    }

	io_u->len = data_len;	
    if ((rw == "write" || rw == "readwrite") && options.verify) {
        memset(io_u->data, butil::fast_rand() % 26 + 'a', data_len);
    }

    if (options.rw_mode == "seq") {
        offset += data_len;
    }
    return io_u;
}

void IOEngine::PutIO(IOUnit *io_u) {
    if (options.verify) {
        memset(io_u->data, '\0', options.block_size);
    }
    io_u_pool->Enqueue(io_u);
}

void EngineFactory::AddEngine(pthread_t pid, const std::string &rw) {
    common::WriteLock lock(engine_lock_);
    engines_[pid].reset(new IOEngine(options_, rw));
    assert(engines_[pid] != nullptr);

    ioctx_pools_[pid].reset(
            new Ring("ioctx_pool", Ring::RING_MP_SC, 1 << 20));
    assert(ioctx_pools_[pid] != nullptr);
    auto s = ioctx_pools_[pid]->Init();
    assert(s.ok());
}

void EngineFactory::WaitEnginesReady(int count) {
    while (true) {
        {
            common::WriteLock lock(engine_lock_);
            if ((int)engines_.size() == count) {
                break;
            }
        }
        usleep(1000);
    }
    return;
}

IOUnit *EngineFactory::GetIO(pthread_t pid) {
    if (!options_.ForbidCrossIo()) return engines_[pid]->GetIO();

    return GetNoneCrossIo(pid);
}

void EngineFactory::PutIO(pthread_t pid, IOUnit *io_u) {
    if (!options_.ForbidCrossIo()) {
        engines_[pid]->PutIO(io_u);
        return;
    }

    PutNoneCrossIo(pid, io_u);
}

IOUnit *EngineFactory::GetNoneCrossIo(pthread_t pid) {
    IOUnit *io_u = nullptr;
    do {
        if (io_u) {
            engines_[pid]->PutIO(io_u);
        }
        io_u = engines_[pid]->GetIO();
    } while (hasCrossIo(io_u));

    return io_u;
}

void EngineFactory::PutNoneCrossIo(pthread_t pid, IOUnit *io_u) {
    common::WriteLock lock(io_lock_);
    inflight_ios_.erase(io_u->offset);
    engines_[pid]->PutIO(io_u);
}

bool EngineFactory::hasCrossIo(IOUnit *io_u) {
    common::WriteLock lock(io_lock_);
	auto lower = inflight_ios_.lower_bound(io_u->offset);
	if (lower != inflight_ios_.end() && (lower->first < (io_u->offset + io_u->len))) {
		return true;
	}
	
	if (lower != inflight_ios_.begin()) {
		--lower;
		if (io_u->offset < (lower->first + lower->second)) {
			return true;
		}
	}

    inflight_ios_.insert(std::make_pair(io_u->offset, io_u->len));
	return false;	
}

}  // namespace clients
}  // namespace cyprestore
