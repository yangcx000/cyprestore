/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_CLIENTS_LIBCYPRE_H_
#define CYPRESTORE_CLIENTS_LIBCYPRE_H_

#include <string>
#include <vector>

#include "libcypre_common.h"
#include "stream/rbd_stream_handle.h"

namespace cyprestore {
namespace clients {

struct CypreRBDOptions {
    CypreRBDOptions(const std::string &eip, int eport)
            : em_ip(eip), em_port(eport), proto(kBrpc),
              brpc_worker_thread_num(9), brpc_sender_thread(4),
              brpc_sender_ring_power(10) {}
    CypreRBDOptions()
            : em_port(0), proto(kBrpc), brpc_worker_thread_num(9),
			  brpc_sender_thread(4), brpc_sender_ring_power(10) {}

    std::string em_ip;
    int em_port;
    ExtentIoProtocol proto;
    int brpc_worker_thread_num;
    // for brpc io sender threads
    int brpc_sender_thread;
    int brpc_sender_ring_power;  // should in [10, 30]
    // brpc sender thread's cpu affinity
    std::vector<int> brpc_sender_thread_cpu_affinity;
};

class CypreRBD {
public:
    CypreRBD() {}
    virtual ~CypreRBD() {}

    // Create a new CypreRBD instance
    static CypreRBD *New();
    // Free all owned resource
    // Will be invoked when delete the rbd instance
    // NOTE: should be called only after all opened handles have been closed.
    virtual int Finalize() = 0;
    virtual int Init(const CypreRBDOptions &opts) = 0;
    // NOT Thread Safe
    // open a per-thread handle
    virtual int
    Open(const std::string &blob_id, RBDStreamHandlePtr &handle) = 0;
    // NOT Thread Safe
    virtual int Close(RBDStreamHandlePtr &handle) = 0;

private:
    CypreRBD(const CypreRBD &);
    void operator=(const CypreRBD &);
};

}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_LIBCYPRE_H_
