/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_CLIENTS_CYPRE_BENCH_H_
#define CYPRESTORE_CLIENTS_CYPRE_BENCH_H_

#include <string>
#include <vector>
//#include "common/cypre_ring.h"
#include "common/ring.h"

namespace cyprestore {
namespace clients {

// using cyprestore::common::CypreRing;

struct Request {
    uint64_t offset;
    uint32_t len;
    char *data;
};

class Mpsc {
public:
    Mpsc(size_t size, const std::string &queue_type, int producers,
         int consumers)
            : size_(size), queue_type_(queue_type), producers_(producers),
              consumers_(consumers), ring_(nullptr) {}
    ~Mpsc() {
        delete ring_;
    }

    int Setup();
    int
    Run(const std::vector<int> &prod_cores, const std::vector<int> &cons_cores);

    // CypreRing *Ring() { return ring_; }
    Ring *ring() {
        return ring_;
    }

private:
    size_t size_;
    std::string queue_type_;
    int producers_;
    int consumers_;
    Ring *ring_;
    // CypreRing *ring_;
};

}  // namespace clients
}  // namespace cyprestore

#endif
