/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_COMMON_EXTENT_ROUTER_H_
#define CYPRESTORE_COMMON_EXTENT_ROUTER_H_

#include <brpc/channel.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/pb/types.pb.h"
#include "common/rwlock.h"

namespace cyprestore {
namespace common {

struct ExtentRouter;
class ExtentRouterMgr;

typedef std::shared_ptr<ExtentRouter> ExtentRouterPtr;
typedef std::unordered_map<std::string, ExtentRouterPtr> ExtentRouterMap;
typedef std::shared_ptr<ExtentRouterMgr> ExtentRouterMgrPtr;

struct ESInstance {
    ESInstance() : public_port(0), private_port(0) {}

    std::string address() const {
        return private_ip + ":" + std::to_string(private_port);
    }

    int es_id;
    std::string public_ip;
    int public_port;
    std::string private_ip;
    int private_port;
};

struct ExtentRouter {
    ExtentRouter() : version(0) {}

    bool IsPrimary(const std::string &ip, int port) {
        return primary.public_ip == ip && primary.public_port == port;
    }

    bool IsSecondary(const std::string &ip, int port) {
        for (size_t i = 0; i < secondaries.size(); ++i) {
            if (secondaries[i].public_ip == ip
                && secondaries[i].public_port == port)
                return true;
        }

        return false;
    }

    bool IsValid(const std::string &ip, int port) {
        return IsPrimary(ip, port) || IsSecondary(ip, port);
    }

    int version;
    std::string extent_id;
    ESInstance primary;
    std::vector<ESInstance> secondaries;
};

class ExtentRouterMgr {
public:
    ExtentRouterMgr(brpc::Channel *em_channel, bool use_thread_local = true)
            : em_channel_(em_channel), use_thread_local_(use_thread_local) {}
    ~ExtentRouterMgr() = default;

    ExtentRouterPtr QueryRouter(const std::string &extent_id);
    void DeleteRouter(const std::string &extent_id);
    void Clear();

private:
    ExtentRouterPtr queryFromRemote(const std::string &extent_id);

    brpc::Channel *em_channel_;
    static thread_local ExtentRouterMap tls_extent_router_map_;
    ExtentRouterMap extent_router_map_;
    RWLock rwlock_;
    bool use_thread_local_;
};

ESInstance toESInstance(const common::pb::EsInstance &pb_es);

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_EXTENT_ROUTER_H_
