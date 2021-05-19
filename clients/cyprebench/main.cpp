/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include <brpc/server.h>
#include <gflags/gflags.h>

#include <iomanip>  // setw
#include <iostream>
#include <vector>

#include "cyprebench.h"
#include "options.h"

DEFINE_string(em_ip, "", "ip address of extentmanager");
DEFINE_int32(em_port, 0, "port of extentmanager");
DEFINE_string(blob_id, "", "blob id");
DEFINE_string(rw, "read", "read/write/readwrite");
DEFINE_string(rw_mode, "seq", "read/write mode, seq or rand");
DEFINE_bool(verify, false, "check data consistency or not");
DEFINE_int32(io_depth, 1, "io queue depth");
DEFINE_int32(read_jobs, 0, "num of read jobs");
DEFINE_int32(write_jobs, 0, "num of write jobs");
DEFINE_string(
        read_jobs_coremask, "4,6,8,10",
        "coremask of read jobs, format:a,b,c,d");
DEFINE_string(
        write_jobs_coremask, "4,6,8,10", "coremask write jobs, format:a,b,c,d");
DEFINE_int32(block_size, 4096, "block size");
DEFINE_uint64(size, 0, "read/write size");
DEFINE_uint64(io_nums, 0, "i/o nums to dispatch");
DEFINE_int32(dummy_server_port, 0, "dummy server port [80]");
DEFINE_bool(run_forever, false, "running forever");
DEFINE_bool(use_nullio, false, "use null io for test");
DEFINE_int32(brpc_sender_ring_power, 16, "brpc sender ring power [16]");
DEFINE_int32(brpc_sender_thread_num, 4, "brpc sender thread number [4]");
DEFINE_int32(brpc_worker_thread_num, 9, "brpc worker thread number [9]");
DEFINE_string(
        brpc_sender_thread_affinity, "12,14,16,18",
        "brpc sender thread affinity, format:a,b,c,d");

static void Usage() {
    std::cout << "Usage: Cyprebench"
              << "\n  -em_ip=[string]"
              << "\n  -em_port=8080"
              << "\n  -blob_id=[string]"
              << "\n  -rw=[read|write|readwrite]"
              << "\n  -rw_mode=[seq|rand]"
              << "\n  -verify=[true|false]"
              << "\n  -io_depth=1"
              << "\n  -read_jobs=0"
              << "\n  -write_jobs=0"
              << "\n  -read_jobs_coremask=4,6,8,10"
              << "\n  -write_jobs_coremask=4,6,8,10"
              << "\n  -block_size=4096"
              << "\n  -size=0"
              << "\n  -io_nums=0"
              << "\n  -run_forever=[true|false]"
              << "\n  -use_nullio=[true|false]"
              << "\n  -brpc_sender_ring_power=[10~30]"
              << "\n  -brpc_sender_thread_num=[4]"
              << "\n  -brpc_worker_thread_num=[9]"
              << "\n  -brpc_sender_thread_affinity=4,6,8,10"
              << "\n  -dummy_server_port=[80|8080]" << std::endl;
}

using namespace cyprestore::clients;

static std::vector<int> ParseCoremask(const std::string &coremask) {
    std::vector<int> cores;
    if (coremask.empty()) {
        return cores;
    }

    int start = 0, end = coremask.find(",");
    while (true) {
        std::string core = coremask.substr(start, end);
        cores.push_back(atoi(core.data()));
        if (end == (int)std::string::npos) {
            break;
        }
        start = end + 1;
        end = coremask.find(",", start);
    }

    return cores;
}

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_em_ip.empty() || FLAGS_em_port == 0 || FLAGS_blob_id.empty()
        || (FLAGS_rw == "read" && FLAGS_write_jobs != 0)
        || (FLAGS_rw == "read" && FLAGS_read_jobs == 0)
        || (FLAGS_rw == "write" && FLAGS_read_jobs != 0)
        || (FLAGS_rw == "write" && FLAGS_write_jobs == 0)) {
        Usage();
        return -1;
    }

    if (FLAGS_dummy_server_port != 0) {
        brpc::StartDummyServerAt(FLAGS_dummy_server_port);
    }

    CyprebenchOptions options;
    options.em_ip = FLAGS_em_ip;
    options.em_port = FLAGS_em_port;
    options.blob_id = FLAGS_blob_id;
    options.rw = FLAGS_rw;
    options.rw_mode = FLAGS_rw_mode;
    options.verify = FLAGS_verify;
    options.io_depth = FLAGS_io_depth;
    options.read_jobs = FLAGS_read_jobs;
    options.write_jobs = FLAGS_write_jobs;
    options.read_jobs_coremask = ParseCoremask(FLAGS_read_jobs_coremask);
    options.write_jobs_coremask = ParseCoremask(FLAGS_write_jobs_coremask);
    options.block_size = FLAGS_block_size;
    options.size = FLAGS_size;
    options.io_nums = FLAGS_io_nums;
    options.run_forever = FLAGS_run_forever;
    options.nullio = FLAGS_use_nullio;
    options.brpc_sender_ring_power = FLAGS_brpc_sender_ring_power;
    options.brpc_sender_thread_num = FLAGS_brpc_sender_thread_num;
    options.brpc_worker_thread_num = FLAGS_brpc_worker_thread_num;
    options.brpc_sender_thread_cpu_affinity =
            ParseCoremask(FLAGS_brpc_sender_thread_affinity);

    Cyprebench &bench = Cyprebench::GlobalCyprebench();
    if (bench.Init(options) != 0) {
        return -1;
    }
    bench.Run();

    return 0;
}
