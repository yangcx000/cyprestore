/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_EXTENTMANAGER_H_
#define CYPRESTORE_EXTENTMANAGER_EXTENTMANAGER_H_

#include <brpc/channel.h>
#include <brpc/server.h>
#include <butil/macros.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "common/config.h"
#include "extent_router.h"
#include "gc_manager.h"
#include "kvstore/rocks_store.h"
#include "pool.h"
#include "user.h"

namespace cyprestore {
namespace extentmanager {

void SignHandler(int sig);

class ExtentManager {
public:
    ExtentManager();
    ~ExtentManager();

    static ExtentManager &GlobalInstance();
    static void HeartbeatFunc(void *arg);

    int Init(const std::string &config_path);
    int RegisterServices();
    void Start();
    int DoRecovery();
    void WaitToQuit();
    void Destroy();

    int InstID() const {
        return inst_id_;
    }
    std::string InstName() const {
        return inst_name_;
    }

    int get_set_id() const {
        return set_id_;
    }
    std::string get_set_name() const {
        return set_name_;
    }

    uint64_t get_extent_size();

    const std::shared_ptr<PoolManager> get_pool_mgr() const {
        return pool_mgr_;
    }
    const std::shared_ptr<UserManager> get_user_mgr() const {
        return user_mgr_;
    }
    const std::shared_ptr<RouterManager> get_router_mgr() const {
        return router_mgr_;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ExtentManager);
    void ReportHeartbeat();

    int inst_id_;
    std::string inst_name_;

    int set_id_;
    std::string set_name_;

    std::shared_ptr<brpc::Channel> sm_channel_;
    std::shared_ptr<PoolManager> pool_mgr_;
    std::shared_ptr<UserManager> user_mgr_;
    std::shared_ptr<RouterManager> router_mgr_;
    std::shared_ptr<GcManager> gc_mgr_;
    std::shared_ptr<kvstore::RocksStore> kv_store_;

    brpc::Server server_;
    volatile bool stop_;
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_EXTENTMANAGER_H_
