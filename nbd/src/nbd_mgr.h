/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_NBDTOOL_H_
#define NBD_SRC_NBDTOOL_H_

#include <memory>
#include <string>
#include <vector>

#include "config.h"
#include "nbd_ctrl.h"
#include "nbd_server.h"
#include "nbd_watch.h"
#include "texttable.h"

namespace cyprestore {
namespace nbd {

class NBDSocketPair {
public:
    NBDSocketPair() : inited_(false) {}
    ~NBDSocketPair() {
        Uninit();
    }

    int Init() {
        if (inited_) {
            return 0;
        }
        int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd_);
        if (ret < 0) {
            return -errno;
        }
        inited_ = true;
        return 0;
    }

    void Uninit() {
        if (inited_) {
            close(fd_[0]);
            close(fd_[1]);
            inited_ = false;
        }
    }

    int First() {
        return fd_[0];
    }

    int Second() {
        return fd_[1];
    }

private:
    bool inited_;
    int fd_[2];
};

// nbd工具的管理模块，负责与nbd server的启动，与nbd内核模块的通信建立
class NBDMgr {
public:
    NBDMgr();
    ~NBDMgr();

    // 与nbd内核建立通信，将文件映射到本地
    int Connect(NBDConfig *cfg);
    // 根据设备路径卸载已经映射的文件
    int Disconnect(const std::string &devpath);
    // 获取已经映射文件的映射信息，包括进程pid、文件名、设备路径
    int List(std::vector<DeviceInfo> *infos, bool show_err = false);
    // 阻塞直到nbd server退出
    void RunServerUntilQuit();

private:
    // 获取指定类型的nbd controller
    NBDControllerPtr GetController(bool try_netlink);

    NBDControllerPtr nbd_ctrl_;

    // 启动nbd server
    // NBDServerPtr StartServer(int sockfd, NBDControllerPtr nbdCtrl,	BlobPtr
    // imageInstance);
    clients::CypreRBD *cyprerbd_;

    // 生成image instance
    BlobPtr GenerateBlob(
            const std::string &extent_manager_ip, int extent_manager_port,
            clients::CypreRBD *cyprerbd);

private:
    std::vector<NBDSocketPair *> socket_pairs_;
    std::vector<NBDServerPtr> nbd_servers_;
    std::shared_ptr<NBDWatchContext> nbd_watch_ctx_;
};

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_NBDTOOL_H_
