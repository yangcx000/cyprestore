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

#include "mpsc.h"

DEFINE_int32(dummy_server_port, 0, "dummy server port [80]");
DEFINE_int32(num_producers, 1, "number of producers");
DEFINE_int32(num_consumers, 1, "number of consumers");
DEFINE_int64(queue_size, 12, "size of queue");
DEFINE_string(queue_type, "spsc", "queue type");
DEFINE_string(producer_cores, "0-3", "producer thread affinity");
DEFINE_string(consumer_cores, "12-17", "consumer thread affinity");

using namespace cyprestore::clients;

std::vector<int> ParseThreadAffinity(const std::string &thread_affinity) {
    std::vector<int> cores;
    int start = 0, end = thread_affinity.find(",");
    while (true) {
        std::string core = thread_affinity.substr(start, end);
        cores.push_back(atoi(core.data()));
        if (end == (int)std::string::npos) {
            break;
        }
        start = end + 1;
        end = thread_affinity.find(",", start);
    }

    return cores;
}

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_dummy_server_port != 0) {
        brpc::StartDummyServerAt(FLAGS_dummy_server_port);
    }

    const std::vector<int> &prod_cores =
            ParseThreadAffinity(FLAGS_producer_cores);
    const std::vector<int> &cons_cores =
            ParseThreadAffinity(FLAGS_consumer_cores);

    Mpsc *mpsc = new Mpsc(
            1 << FLAGS_queue_size, FLAGS_queue_type, FLAGS_num_producers,
            FLAGS_num_consumers);
    if (mpsc->Setup() != 0) {
        return -1;
    }
    mpsc->Run(prod_cores, cons_cores);

    return 0;
}
