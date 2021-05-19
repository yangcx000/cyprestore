/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_BLOBINSTANCE_H_
#define NBD_SRC_BLOBINSTANCE_H_

#include <butil/logging.h>
#include <linux/nbd.h>

#include <memory>
#include <mutex>
#include <string>
#include <stdlib.h>

#include "libcypre.h"

namespace cyprestore {
namespace nbd {

const int CYPRE_BLOCK_SIZE = 4096;

class NBDServer;
// NBD IO请求上下文信息
struct IOContext {
    struct nbd_request request;
    struct nbd_reply reply;

    // 请求类型
    int command = 0;
    NBDServer* server = nullptr;
    std::unique_ptr<char[]> data;

    // 对齐的offset
    uint64_t align_offset = 0;
    // 对齐的length
    uint64_t align_length = 0;
    // 异步返回的返回值
    int ret = 0;
    // 异步请求的回调函数
    clients::io_completion_cb cb = nullptr;

    struct timespec start_time;
    struct timespec end_time;
};

typedef IOContext *IOContextPtr;

// 封装blob相关接口
class BlobInstance {
public:
    BlobInstance(
            const std::string &em_ip, int em_port, clients::CypreRBD *cyprerbd);

    virtual ~BlobInstance();

    /**
     * @brief 打开卷
     * @return 打开成功返回 0
     *         打开失败返回错误码
     */
    virtual int Open(const std::string &blob_id);

    /**
     * @brief 关闭卷
     */
    virtual void Close();

    /**
     * @brief 异步读请求
     * @param ctx 读请求信息
     */
    virtual bool Read(IOContext *ctx);

    /**
     * @brief 异步写请求
     * @param ctx 写请求信息
     */
    virtual bool Write(IOContext *ctx, bool flush = false);

    /**
     * @brief trim请求
     * @param ctx trim请求信息
     */
    virtual void Trim(IOContext *ctx);

    /**
     * @brief flush请求
     * @param ctx flush请求信息
     */
    virtual void Flush(IOContext *ctx);

    /**
     * @brief 获取文件大小
     * @return 获取成功返回文件大小（正值）
     *         获取失败返回错误码（负值）
     */
    virtual uint64_t GetBlobSize();
private:
    // cypre
    clients::CypreRBD *cyprerbd_;
    cyprestore::clients::RBDStreamHandlePtr handle_;

    int fd_;
    // 卷名
    std::string blobid_;
    std::string extent_manager_ip_;
    int extent_manager_port_;

    bool TestMemRead(IOContext *ctx);
    bool TestMemWrite(IOContext *ctx);

    bool TestFileRead(IOContext *ctx);
    bool TestFileWrite(IOContext *ctx, bool flush = false);

    bool TestAioRead(IOContext *ctx);
    bool TestAioWrite(IOContext *ctx);

public:
    bool btestfile_;
    bool btestmem_;
};
using BlobPtr = std::shared_ptr<BlobInstance>;

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_BLOBINSTANCE_H_
