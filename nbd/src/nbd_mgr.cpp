/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#include "nbd_mgr.h"

#include <limits.h>

#include <memory>
#include <string>

#include "argparse.h"
#include "blob_instance.h"
#include "common/error_code.h"
#include "plus.h"

namespace cyprestore {
namespace nbd {

extern std::shared_ptr<NBDConfig> g_nbd_config;

NBDMgr::NBDMgr() {
    cyprerbd_ = nullptr;
}

NBDMgr::~NBDMgr() {
    for (unsigned int i = 0; i < nbd_servers_.size(); i++) {
        NBDServerPtr server_ptr = nbd_servers_.at(i);
        server_ptr->Shutdown();
    }
    delete cyprerbd_;  //要放在ServerPtr->Shutdown() 析构之后

    for (unsigned int i = 0; i < socket_pairs_.size(); i++) {
        NBDSocketPair *socket_pair = socket_pairs_.at(i);
        delete socket_pair;
    }

    socket_pairs_.clear();
}

NBDControllerPtr NBDMgr::GetController(bool try_netlink) {
    if (try_netlink) {
        auto ctrl = std::make_shared<NetLinkController>();
        bool support_netLink = ctrl->Support();
        if (support_netLink) {
            return ctrl;
        }
    }
    return std::make_shared<IOController>();
}

int NBDMgr::Connect(NBDConfig *cfg) {
    cyprerbd_ = clients::CypreRBD::New();
    // loadmodule 到时候放到外面做

    // init socket pair
    std::vector<int> sock_firsts;
    std::vector<int> sock_seconds;
    int ret = 0;

    for (int i = 0; i < cfg->multi_conns; i++) {
        NBDSocketPair *socket_pair = new NBDSocketPair();
        ret = socket_pair->Init();
        if (ret < 0) {
            delete socket_pair;
            return ret;
        }

        sock_firsts.push_back(socket_pair->First());
        sock_seconds.push_back(socket_pair->Second());
        socket_pairs_.push_back(socket_pair);
    }

    std::string em_ip, em_port_str;
    std::string em = cfg->em_endpoint;
    SplitStr(cfg->em_endpoint, ':', em_ip, em_port_str);
    int em_port = 0;
    if (em_port_str.length() > 0) em_port = std::stoi(em_port_str);

    // load nbd module
    ret = LoadModule(cfg);
    if (ret < 0) {
        return ret;
    }

    nbd_ctrl_ = GetController(cfg->try_netlink);

    if (em_ip.length() > 0) {
        clients::CypreRBDOptions opt(em_ip.c_str(), em_port);

        LOG(NOTICE) << "NBDMgr::Connect g_nbd_config->nullio= "
                  << g_nbd_config->nullio << " opt.proto=" << opt.proto;
        if (g_nbd_config->nullio) {
            opt.proto = clients::kNull;
        }

        std::vector<std::string> strarr;
        StringSplit(cfg->client_core_mask, ",", strarr);
        for (uint i = 0; i < strarr.size(); ++i)
            opt.brpc_sender_thread_cpu_affinity.push_back(std::stoi(strarr[i]));
        opt.brpc_sender_ring_power = 20;
        opt.brpc_sender_thread = 16;

        ret = cyprerbd_->Init(opt);
        if (ret != cyprestore::common::CYPRE_OK) {
            LOG(ERROR) << "cypre_rbd_->init failed ";
            return ret;
        }
    }

    int64_t file_size = 0;

    for (unsigned int i = 0; i < sock_seconds.size(); i++) {
        // 初始化打开文件
        BlobPtr blob_instance = GenerateBlob(em_ip, em_port, cyprerbd_);
        ret = blob_instance->Open(cfg->imgname);
        if (ret < 0) {
            LOG(ERROR) << "cypre_ndb: Could not open image.";
            return ret;
        }

        // 判断文件大小是否符合预期
        if (i == 0) {
            file_size = blob_instance->GetBlobSize();
            if (file_size <= 0) {
                LOG(ERROR) << "cypre_ndb: Get file size failed.";
                return -1;
            }
        }

        NBDServerPtr nbd_server =
                std::make_shared<NBDServer>(sock_seconds.at(i), blob_instance);
        nbd_servers_.push_back(nbd_server);
    }

    // setup controller
    uint64_t flags = NBD_FLAG_SEND_FLUSH | NBD_FLAG_SEND_TRIM
                     | NBD_FLAG_HAS_FLAGS | NBD_FLAG_CAN_MULTI_CONN;
    if (cfg->readonly) {
        flags |= NBD_FLAG_READ_ONLY;
    }
    ret = nbd_ctrl_->SetUp(cfg, sock_firsts, file_size, flags);
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: nbd_ctrl_->SetUp  failed";
        return ret;
    }

    BlobPtr blob_instance = GenerateBlob(em_ip, em_port, cyprerbd_);
    ret = blob_instance->Open(cfg->imgname);
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: Could not open image.";
        return ret;
    }

    nbd_watch_ctx_ = std::make_shared<NBDWatchContext>(
            nbd_ctrl_, blob_instance, file_size);

    return 0;
}

int NBDMgr::Disconnect(const std::string &devpath) {
    NBDControllerPtr nbd_ctrl = GetController(false);
    return nbd_ctrl->DisconnectByPath(devpath);
}

int NBDMgr::List(std::vector<DeviceInfo> *infos, bool show_err) {
    DeviceInfo info;
    NBDListIterator it(show_err);
    while (it.Get(&info)) {
        infos->push_back(info);
    }
    return 0;
}

void NBDMgr::RunServerUntilQuit() {
    // start nbd server
    for (unsigned int i = 0; i < nbd_servers_.size(); i++) {
        nbd_servers_.at(i)->Start();
    }

    // start watch context
    nbd_watch_ctx_->WatchImageSize();

    // NBDControllerPtr ctrl = nbdServer_->GetController();
    if (nbd_ctrl_->IsNetLink()) {
        LOG(NOTICE) << "NBDMgr::RunServerUntilQuit "
                     "nbdServer_->WaitForDisconnect...";

        for (unsigned int i = 0; i < nbd_servers_.size(); i++) {
            nbd_servers_.at(i)->WaitForDisconnect();
        }
    } else {
        LOG(NOTICE) << "NBDMgr::RunServerUntilQuit...";
        nbd_ctrl_->RunUntilQuit();
        LOG(NOTICE) << "NBDMgr::RunServerUntilQuit end";
    }
}

BlobPtr NBDMgr::GenerateBlob(
        const std::string &extent_manager_ip, int extent_manager_port,
        clients::CypreRBD *cyprerbd) {
    return std::make_shared<BlobInstance>(
            extent_manager_ip, extent_manager_port, cyprerbd);
}

}  // namespace nbd
}  // namespace cyprestore
