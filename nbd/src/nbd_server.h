/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_NBDSERVER_H_
#define NBD_SRC_NBDSERVER_H_

#include <linux/nbd.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "blob_instance.h"
#include "nbd_ctrl.h"
#include "safe_io.h"
#include "spin_lock.h"

namespace cyprestore {
namespace nbd {

/**
 * @brief NEBD异步请求回调函数
 */
void NBDAioCallback(int rc, void *vctx);

// NBDServer负责与nbd内核进行数据通信
class NBDServer {
public:
    NBDServer(int sock, std::shared_ptr<BlobInstance> blobInstance);

    ~NBDServer();

    /**
     * @brief 启动server
     */
    void Start();

    /**
     * @brief 关闭server
     */
    void Shutdown();

    /**
     * 等待断开连接
     */
    void WaitForDisconnect();

    /**
     * 测试使用，返回server是否终止
     */
    bool IsTerminated() const {
        return terminated_;
    }

    /*
    NBDControllerPtr GetController()
    {
            return nbdCtrl_;
    }
  */
    /**
     * @brief 异步请求结束时执行函数
     * @param ctx 异步请求上下文
     */
    void OnRequestFinish(IOContext *ctx);

    void SendReply(IOContext *ctx);

private:
    bool StartAioRequest(IOContext *ctx);

    /**
     * @brief 读线程执行函数
     */
    void ReaderFunc();

    /**
     * @brief 异步请求开始时执行函数
     */
    void OnRequestStart();

    bool OnRecvPack(
            uint32_t request_type, uint64_t request_from, uint32_t request_len,
            const char *request_handle, const unsigned char *buf,
            uint32_t buflen);
private:
    // 与内核通信的socket fd
    int sock_;
    // 正在执行过程中的请求数量
    std::atomic<uint64_t> pending_request_counts_;

    // server是否启动
    std::atomic<bool> started_;
    // server是否停止
    std::atomic<bool> terminated_;

    spin_lock sock_spin_lock_;

    std::shared_ptr<BlobInstance> blob_;
    std::shared_ptr<SafeIO> safe_io_;

    // 读线程
    std::thread reader_thread_;

    // 等待断开连接锁/条件变量
    std::mutex disconnect_mtx_;
    std::condition_variable disconnect_cond_;
};
using NBDServerPtr = std::shared_ptr<NBDServer>;

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_NBDSERVER_H_
