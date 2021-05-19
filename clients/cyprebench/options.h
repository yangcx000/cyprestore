/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_CLIENTS_OPTIONS_H_
#define CYPRESTORE_CLIENTS_OPTIONS_H_

#include <pthread.h>

#include <atomic>
#include <ctime>
#include <string>
#include <vector>

namespace cyprestore {
namespace clients {

const uint32_t kAlignSize = 4096;
typedef void *(*bootstrap_func)(void *);

class Cyprebench;
struct CyprebenchOptions {
    CyprebenchOptions()
            : em_port(-1), verify(false), io_depth(1), read_jobs(0),
              write_jobs(0), block_size(4096), size(0), io_nums(0),
              run_forever(false), nullio(false), brpc_sender_ring_power(16),
              brpc_sender_thread_num(4), brpc_worker_thread_num(9) {}

    bool ForbidCrossIo() {
        return verify
               && (write_jobs > 1 || (write_jobs != 0 && read_jobs != 0)
                   || io_depth > 1);
    }

    std::string em_ip;
    int em_port;

    std::string blob_id;

    std::string rw;
    std::string rw_mode;
    bool verify;
    int io_depth;
    int read_jobs;
    int write_jobs;
    uint32_t block_size;
    uint64_t size;
    uint64_t io_nums;
	bool run_forever;
    bool nullio;
    int brpc_sender_ring_power;
    int brpc_sender_thread_num;
    int brpc_worker_thread_num;
    std::vector<int> read_jobs_coremask;
    std::vector<int> write_jobs_coremask;
    std::vector<int> brpc_sender_thread_cpu_affinity;
};

struct IOUnit {
    IOUnit(uint64_t offset_, uint32_t len_)
            : offset(offset_), len(len_), data(nullptr) {
        data = new char[len_];
    }

    uint64_t offset;
    uint32_t len;
    char *data;
};

struct IOContext {
    Cyprebench *cypre_bench;
    std::atomic<int> *io_depth;
    IOUnit *io_u;
    pthread_t pid;
    struct timespec start_time;
    struct timespec end_time;
};

}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_OPTIONS_H_
