/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_CLIENTS_CYPREBENCH_H_
#define CYPRESTORE_CLIENTS_CYPREBENCH_H_

#include <pthread.h>

#include <atomic>
#include <string>
#include <vector>

#include "file.h"
#include "libcypre/libcypre.h"
#include "io_engine.h"
#include "options.h"

namespace cyprestore {
namespace clients {

class Cyprebench {
public:
    Cyprebench()
            : stop_(false), engine_factory_(nullptr),
              cypre_rbd_(nullptr), file_(nullptr) {}

    ~Cyprebench() {
        delete engine_factory_;
        delete cypre_rbd_;
        delete file_;
    }

    static Cyprebench &GlobalCyprebench();
    int Init(const CyprebenchOptions &options);
    void Run();
    void Stop();

    IOContext *GetIoContext(pthread_t pid, std::atomic<int> *io_depth);
    void PutIoContext(IOContext *io_ctx);

    CyprebenchOptions *Options() { return &options_; }
    File *GetFile() const { return file_; }

private:
    static void *bootstrapRead(void *arg);
    static void *bootstrapWrite(void *arg);
    static void sigHandler(int signum);
    int launchThreads(const std::string &rw);
    void doRead(RBDStreamHandlePtr handle);
    void doWrite(RBDStreamHandlePtr handle);

    CyprebenchOptions options_;
    std::atomic<bool> stop_;
    EngineFactory *engine_factory_;
    CypreRBD *cypre_rbd_;
    File *file_;
    std::vector<pthread_t> read_jobs_;
    std::vector<pthread_t> write_jobs_;
};

}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_CYPREBENCH_H_
