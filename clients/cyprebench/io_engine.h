/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_CLIENTS_IO_ENGINE_H_
#define CYPRESTORE_CLIENTS_IO_ENGINE_H_

#include <map>
#include <string>
#include <unordered_map>

#include "common/ring.h"
#include "common/rwlock.h"
#include "options.h"

namespace cyprestore {
namespace clients {

struct IOEngine {
    IOEngine(const CyprebenchOptions &options_, const std::string &rw_);
    ~IOEngine();

    IOUnit *GetIO();
    void PutIO(IOUnit *io_u);

    CyprebenchOptions options;
    uint64_t offset;
    uint64_t num_blocks;
    uint64_t num_align_blocks;
    Ring *io_u_pool;
    std::string rw;
};

class EngineFactory {
public:
    EngineFactory(const CyprebenchOptions &options)
			: options_(options) {}
    ~EngineFactory() {}

    void AddEngine(pthread_t pid, const std::string &rw);
    void WaitEnginesReady(int count);

    IOUnit *GetIO(pthread_t pid);
    void PutIO(pthread_t pid, IOUnit *io_u);

    IOUnit *GetNoneCrossIo(pthread_t pid);
    void PutNoneCrossIo(pthread_t pid, IOUnit *io_u);

private:
    friend class Cyprebench;
	bool hasCrossIo(IOUnit *io_u);

    CyprebenchOptions options_;
    common::RWLock engine_lock_;
    std::unordered_map<pthread_t, std::unique_ptr<IOEngine>> engines_;
    std::unordered_map<pthread_t, std::unique_ptr<Ring>> ioctx_pools_;
    std::map<uint64_t, uint32_t> inflight_ios_;
    common::RWLock io_lock_;
};

}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_IO_ENGINE_H_
