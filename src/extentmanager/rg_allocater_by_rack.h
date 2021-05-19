/*
 * Copyright 2020 JDD authors
 * @feifei5
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_RA_BY_RACK_H_
#define CYPRESTORE_EXTENTMANAGER_RA_BY_RACK_H_

#include "rg_allocater.h"

namespace cyprestore {
namespace extentmanager {

class RGAllocaterByRack : public RGAllocater {
public:
    RGAllocaterByRack() = default;
    ~RGAllocaterByRack() = default;

    Status
    init(const std::vector<std::shared_ptr<ExtentServer>> &ess,
         uint64_t extent_size);
    std::shared_ptr<ExtentServer> select_primary();
    std::shared_ptr<ExtentServer> select_secondary(const std::vector<int> &ess);

private:
    std::unordered_map<int, std::shared_ptr<ExtentServer>> es_map_;
    RackHeap racks_;
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_RA_BY_RACK_H_
