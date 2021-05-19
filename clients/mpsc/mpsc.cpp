/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "mpsc.h"

#include <bvar/bvar.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

//#include "spdk_mgr.h"

namespace cyprestore {
namespace clients {

bvar::Adder<uint64_t>
        g_request_consumed("cyprestore_clients", "request_consumed");
bvar::PerSecond<bvar::Adder<uint64_t>> g_request_consumed_second(
        "cyprestore_clients", "request_consumed_second", &g_request_consumed,
        60);
bvar::LatencyRecorder g_consumed_latency("cyprestore_clients", "consumed");
bvar::LatencyRecorder g_producer_latency("cyprestore_clients", "producer");

// using namespace cyprestore::extentserver;

int Mpsc::Setup() {
    /*
    SpdkEnvOptions options;
    options.shm_id = -1;
    options.mem_channel = -1;
    options.mem_size = -1;
    options.master_core = -1;
    options.num_pci_addr = 0;
    options.no_pci = false;
    options.hugepage_single_segments = false;
    options.unlink_hugepage = false;
    options.core_mask = "0xff";
    options.huge_dir = "";
    options.name = "";
    options.config_file = "";

    SpdkMgr *spdk_mgr = new SpdkMgr(options);
    auto s = spdk_mgr->init_spdk_env();
    if (!s.ok()) {
        std::cout << "Couldn't init_spdk_env, err_msg:" << s.ToString() <<
    std::endl; return -1;
    }
    */

    // ring_ = new CypreRing("mpsc", CypreRing::TYPE_MP_MC, size_);
    Ring::RingType ring_type = Ring::RING_SP_SC;
    if (queue_type_ == "mpsc") {
        ring_type = Ring::RING_MP_SC;
    } else if (queue_type_ == "spmc") {
        ring_type = Ring::RING_SP_MC;
    } else if (queue_type_ == "mpmc") {
        ring_type = Ring::RING_MP_MC;
    }

    ring_ = new Ring("mpsc", ring_type, size_);
    if (!ring_) {
        std::cout << "Couldn't alloc CypreRing, size:" << size_ << std::endl;
        return -1;
    }
    auto s = ring_->Init();
    if (!s.ok()) {
        std::cout << "Couldn't init CypreRing, err_msg:" << s.ToString()
                  << std::endl;
        return -1;
    }
    return 0;
}

static void *producer_func(void *arg) {
    Mpsc *queue = static_cast<Mpsc *>(arg);
    Request *req = nullptr;
    while (true) {
        // if (queue->Ring()->Enqueue((void **)&req) != 1) {
        if (!queue->ring()->Enqueue((void **)&req).ok()) {
            // std::cout << "Couldn't push request" << std::endl;
            continue;
        }
        g_producer_latency << 1;
    }

    return nullptr;
}

static void *consumer_func(void *arg) {
    Mpsc *queue = static_cast<Mpsc *>(arg);
    // Request *reqs[16];
    Request *req = nullptr;
    int fd = open("/dev/null", O_WRONLY);
    while (true) {
        /*
        size_t n = queue->Ring()->DequeueBurst((void **)&reqs, 16);
        if (n != 0) {
            g_request_consumed << n;
        }
        */
        // if (queue->Ring()->Dequeue((void **)&req) != 1) {
        if (!queue->ring()->Dequeue((void **)&req).ok()) {
            continue;
        }
        g_consumed_latency << 1;
        // write(fd, "c", 1);
    }

    return nullptr;
}

int Mpsc::Run(
        const std::vector<int> &prod_cores,
        const std::vector<int> &cons_cores) {
    int ret = 0;

    std::vector<pthread_t> tids_consumers(consumers_);
    for (size_t i = 0, icpu = 0; i < tids_consumers.size(); ++i, ++icpu) {
        ret = pthread_create(&tids_consumers[i], nullptr, consumer_func, this);
        if (ret != 0) {
            std::cout << "Couldn't create pthread" << std::endl;
            return -1;
        }
        if (cons_cores.empty()) {
            continue;
        }

        if (icpu >= cons_cores.size()) {
            icpu = 0;
        }

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cons_cores[icpu], &cpuset);
        ret = pthread_setaffinity_np(
                tids_consumers[i], sizeof(cpuset), &cpuset);
        if (ret != 0) {
            std::cout << "Couldn't set consumer thread affinity, ret=" << ret;
            return -1;
        }
    }

    std::vector<pthread_t> tids(producers_);
    for (size_t i = 0, icpu = 0; i < tids.size(); ++i, ++icpu) {
        ret = pthread_create(&tids[i], nullptr, producer_func, this);
        if (ret != 0) {
            std::cout << "Couldn't create pthread" << std::endl;
            return -1;
        }

        if (prod_cores.empty()) {
            continue;
        }

        if (icpu >= prod_cores.size()) {
            icpu = 0;
        }

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(prod_cores[icpu], &cpuset);
        ret = pthread_setaffinity_np(tids[i], sizeof(cpuset), &cpuset);
        if (ret != 0) {
            std::cout << "Couldn't set producer thread affinity, ret=" << ret;
            return -1;
        }
    }

    for (size_t i = 0; i < tids_consumers.size(); ++i) {
        pthread_join(tids_consumers[i], nullptr);
    }

    for (size_t i = 0; i < tids.size(); ++i) {
        pthread_join(tids[i], nullptr);
    }

    std::cout << "Mpsc run to completion" << std::endl;
    return 0;
}

}  // namespace clients
}  // namespace cyprestore
