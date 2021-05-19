/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */
#ifndef CYPRESTORE_ACCESS_ACCESS_MANAGER_H
#define CYPRESTORE_ACCESS_ACCESS_MANAGER_H

#include <brpc/server.h>
#include <time.h>

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include "common/config.h"
#include "common/pb/types.pb.h"
#include "resource_manager.h"
#include "set_manager.h"

namespace cyprestore {
namespace access {

void SignHandler(int sig);

class Access {
public:
    Access() = default;
    ~Access() = default;

    static Access &GlobalInstance();
    int Init(const std::string &config_path);
    int RegistreServices();
    void Start();
    void WaitToQuit();
    void Destroy();

    std::shared_ptr<SetMgr> get_set_mgr() {
        return set_mgr_;
    }
    std::shared_ptr<ResourceMgr> get_resource_mgr() {
        return resource_mgr_;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(Access);

    // int LastUpdateSetMapTime;
    // bool UpdateSetMap();
    // SetPtr GetSetPtr(const std::string &set_name);
    //
    int InitSetManager();
    int InitExtentManager();

    brpc::Server server_;
    std::shared_ptr<SetMgr> set_mgr_;
    std::shared_ptr<ResourceMgr> resource_mgr_;
    volatile bool stop_;
};

}  // namespace access
}  // namespace cyprestore

#endif  // CYPRESTORE_ACCESS_ACCESS_MANAGER_H
