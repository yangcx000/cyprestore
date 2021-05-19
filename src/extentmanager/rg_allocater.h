/*
 * Copyright 2020 JDD authors
 * @feifei5
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_RA_H_
#define CYPRESTORE_EXTENTMANAGER_RA_H_

#include <list>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/error_code.h"
#include "common/status.h"
#include "extent_server.h"

namespace cyprestore {
namespace extentmanager {

struct Stat {
    Stat() : rgs(0), primary_rgs(0), secondary_rgs(0), weight(0) {}

    int rgs;
    int primary_rgs;
    int secondary_rgs;
    uint64_t weight;
};

typedef std::shared_ptr<ExtentServer> ExtentServerPtr;

// es
struct EsCmp {
    bool operator()(const ExtentServerPtr &lhs, const ExtentServerPtr &rhs) {
        return (1000000 * lhs->get_sec_rgs() / lhs->get_weight()
                > (1000000 * rhs->get_sec_rgs() / rhs->get_weight()))
               || (1000000 * lhs->get_pri_rgs() / lhs->get_weight())
                          > (1000000 * rhs->get_pri_rgs() / rhs->get_weight());
    }
};
typedef std::priority_queue<
        ExtentServerPtr, std::vector<ExtentServerPtr>, EsCmp>
        EsHeap;

// host
struct Host {
    Host(const std::string &name_) : name(name_) {}

    std::string name;
    EsHeap ess;
    Stat stat;
};

typedef std::shared_ptr<Host> HostPtr;
// host cmp
struct HostCmp {
    bool operator()(const HostPtr &lhs, const HostPtr &rhs) {
        return 1000000 * lhs->stat.secondary_rgs / lhs->stat.weight
                       > 1000000 * rhs->stat.secondary_rgs / rhs->stat.weight
               || 1000000 * lhs->stat.primary_rgs / lhs->stat.weight
                          > 1000000 * rhs->stat.primary_rgs / rhs->stat.weight;
    }
};
typedef std::priority_queue<HostPtr, std::vector<HostPtr>, HostCmp> HostHeap;

// rack
struct Rack {
    Rack(const std::string &name_) : name(name_) {}

    std::string name;
    HostHeap hosts;
    Stat stat;
};
typedef std::shared_ptr<Rack> RackPtr;
typedef std::list<RackPtr> RackList;
// rack cmp
struct RackCmp {
    bool operator()(const RackPtr &lhs, const RackPtr &rhs) {
        return 1000000 * lhs->stat.secondary_rgs / lhs->stat.weight
                       > 1000000 * rhs->stat.secondary_rgs / rhs->stat.weight
               || 1000000 * lhs->stat.primary_rgs / lhs->stat.weight
                          > 1000000 * rhs->stat.primary_rgs / rhs->stat.weight;
    }
};
typedef std::priority_queue<RackPtr, std::vector<RackPtr>, RackCmp> RackHeap;

using common::Status;

class RGAllocater {
public:
    RGAllocater() = default;
    virtual ~RGAllocater() = default;

    virtual Status
    init(const std::vector<std::shared_ptr<ExtentServer>> &ess,
         uint64_t extent_size) = 0;
    virtual std::shared_ptr<ExtentServer> select_primary() = 0;
    virtual std::shared_ptr<ExtentServer>
    select_secondary(const std::vector<int> &ess) = 0;
    Status
    host_rack_valid(const std::vector<std::shared_ptr<ExtentServer>> &ess) {
        std::unordered_map<std::string, std::string> host_rack_map;
        for (auto &es : ess) {
            if (host_rack_map.find(es->get_host()) != host_rack_map.end()) {
                if (host_rack_map[es->get_host()] != es->get_rack()) {
                    LOG(ERROR)
                            << "Rack and Host invalid, host:" << es->get_host()
                            << ", rack:" << es->get_rack();
                    return Status(common::CYPRE_ER_INVALID_ARGUMENT);
                }
            } else {
                host_rack_map[es->get_host()] = es->get_rack();
            }
        }
        return Status();
    }
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_RA_H_
