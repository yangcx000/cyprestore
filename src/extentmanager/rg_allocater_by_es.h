/*
 * Copyright 2020 JDD authors
 * @feifei5
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_RA_BY_ES_H_
#define CYPRESTORE_EXTENTMANAGER_RA_BY_ES_H_

#include "rg_allocater.h"

namespace cyprestore {
namespace extentmanager {

using common::Status;

class RGAllocaterByEs : public RGAllocater {
public:
    RGAllocaterByEs() = default;
    ~RGAllocaterByEs() = default;

    Status
    init(const std::vector<std::shared_ptr<ExtentServer>> &ess,
         uint64_t extent_size);
    std::shared_ptr<ExtentServer> select_primary();
    std::shared_ptr<ExtentServer> select_secondary(const std::vector<int> &ess);

private:
    EsHeap ess_;
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_RA_BY_ES_H_
