/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_NBDCONTROLLER_H_
#define NBD_SRC_NBDCONTROLLER_H_

#include <linux/fs.h>
#include <linux/nbd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/signal.h>
// 需要安装 libnl-3-dev libnl-genl-3-dev package
#include <butil/logging.h>
#include <libnl3/netlink/genl/ctrl.h>
#include <libnl3/netlink/genl/genl.h>
#include <libnl3/netlink/genl/mngt.h>
#include <pthread.h>

#include <memory>
#include <string>
#include <thread>

#include "config.h"
#include "nbd_netlink.h"
#include "util.h"

namespace cyprestore {
namespace nbd {

// 控制NBD内核模块，包括与内核模块通信连接的建立和断开
class NBDController {
public:
    NBDController() : nbd_fd_(-1), nbd_index_(-1) {}
    virtual ~NBDController() {
        ClearUp();
    }

    /**
     * @brief: 安装NBD设备，并初始化设备属性
     * @param config: 启动NBD设备相关的配置参数
     * @param sockfd:
     * socketpair其中一端的fd，传给NBD设备用于跟NBDServer间的数据传输
     * @param size: 设置NBD设备的大小
     * @param flags: 设置加载NBD设备的flags
     * @return: 成功返回0，失败返回负值
     */
    virtual int
    SetUp(NBDConfig *config, const std::vector<int> &socks, uint64_t size,
          uint64_t flags) = 0;
    /**
     * @brief: 根据设备名来卸载已经映射的NBD设备
     * @param devpath: 设备路径，例如/dev/nbd0
     * @return: 成功返回0，失败返回负值
     */
    virtual int DisconnectByPath(const std::string &devpath) = 0;
    /**
     * @brief: 重新设置nbd设备对外显示的大小，可通过lsblk查看
     * @param size: 需要更新的设备的大小
     * @return: 成功返回0，失败返回负值
     */
    virtual int Resize(uint64_t size) = 0;

    // 如果nbd设备已经加载，这里会阻塞，直到NBD出现异常或者收到disconnect命令
    virtual void RunUntilQuit() {
        if (nbd_fd_ < 0) {
            return;
        }
        std::thread th = std::thread([&]() {
            // ignore all signals, ensure ioctl exit only by unmap
            sigset_t mask;
            ::sigfillset(&mask);
            ::pthread_sigmask(SIG_BLOCK, &mask, NULL);

            std::string devpath = DEV_PATH_PREFIX + std::to_string(nbd_index_);
            ioctl(nbd_fd_, NBD_DO_IT);
            LOG(NOTICE) << "NBDController::RunUntilQuit end " << strerror(errno)
                      << ", nbd_fd_=" << nbd_fd_;
        });
        th.join();
    }

    // 清理释放资源
    void ClearUp() {
        if (nbd_fd_ < 0) {
            return;
        }
        close(nbd_fd_);
        nbd_fd_ = -1;
        nbd_index_ = -1;
    }

    // 获取NBD设备的index，该值就是设备路径/dev/nbd{num}中的num
    int GetNBDIndex() {
        return nbd_index_;
    }

    // 用来获取当前的Controller是不是netlink形式与内核通信
    virtual bool IsNetLink() {
        return false;
    }

    // 如果flag中带NBD_FLAG_READ_ONLY，将nbd设备设置为只读模式
    int CheckSetReadOnly(int nbdfd, int flag) {
        int arg = 0;
        if (flag & NBD_FLAG_READ_ONLY) {
            arg = 1;
        }
        int ret = ioctl(nbdfd, BLKROSET, (unsigned long)&arg);  // NOLINT
        if (ret < 0) {
            ret = -errno;
        }
        return ret;
    }

protected:
    int nbd_fd_;
    int nbd_index_;
};
using NBDControllerPtr = std::shared_ptr<NBDController>;

class IOController : public NBDController {
public:
    IOController() {}
    ~IOController() {}

    int
    SetUp(NBDConfig *config, const std::vector<int> &socks, uint64_t size,
          uint64_t flags) override;
    int DisconnectByPath(const std::string &devpath) override;
    int Resize(uint64_t size) override;

private:
    int InitDevAttr(
            int devfd, NBDConfig *config, const std::vector<int> &socks,
            uint64_t size, uint64_t flags);
};

class NetLinkController : public NBDController {
public:
    NetLinkController() : nl_id_(-1), sock_(nullptr) {}
    ~NetLinkController() {}

    int
    SetUp(NBDConfig *config, const std::vector<int> &socks, uint64_t size,
          uint64_t flags) override;
    int DisconnectByPath(const std::string &devpath) override;
    int Resize(uint64_t size) override;
    bool Support();

    bool IsNetLink() override {
        return true;
    }

private:
    int Init();
    void Uninit();
    int ConnectInternal(
            NBDConfig *config, int sockfd, uint64_t size, uint64_t flags);
    int DisconnectInternal(int index);
    int ResizeInternal(int nbdIndex, uint64_t size);

private:
    int nl_id_;
    struct nl_sock *sock_;
};

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_NBDCONTROLLER_H_
