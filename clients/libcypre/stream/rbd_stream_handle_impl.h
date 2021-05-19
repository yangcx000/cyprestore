
/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 */

#ifndef CYPRESTORE_CLIENTS_STREAM_RBD_STREAM_HANDLE_IMPL_H_
#define CYPRESTORE_CLIENTS_STREAM_RBD_STREAM_HANDLE_IMPL_H_

#include <memory>
#include <mutex>
#include <unordered_map>

#include "common/connection_pool.h"
//#include "concurrency/io_concurrency.h"
#include "stream/extent_stream_handle.h"
#include "stream/rbd_stream_handle.h"

namespace cyprestore {
namespace clients {

class BrpcSenderWorker;
struct RBDStreamOptions {
    RBDStreamOptions(
            const std::string &id, const std::string &name,
            const std::string &pool, const std::string &user, uint64_t es,
            uint64_t bs)
            : blob_id(id), blob_name(name), pool_id(pool), user_id(user),
              extent_size(es), blob_size(bs), max_iosize(1024 * 1024 * 16),
              optimal_iosize(0), conn_pool(NULL), brpc_sender(NULL) {}
    RBDStreamOptions()
            : extent_size(0), blob_size(0), max_iosize(1024 * 1024 * 16),
              optimal_iosize(0), conn_pool(NULL), brpc_sender(NULL) {}

    std::string blob_id;    // blob id
    std::string blob_name;  // blob name
    std::string pool_id;
    std::string user_id;
    uint64_t extent_size;        // extent size
    mutable uint64_t blob_size;  // blob size
    mutable uint64_t max_iosize;
    mutable uint64_t optimal_iosize;
    //
    mutable std::shared_ptr<common::ConnectionPool2> conn_pool;
    mutable common::ExtentRouterMgrPtr extent_router_mgr;
    BrpcSenderWorker *brpc_sender;
};

class RBDStreamHandleImpl : public RBDStreamHandle {
public:
    RBDStreamHandleImpl(const RBDStreamOptions &opt);
    virtual ~RBDStreamHandleImpl();

    virtual int Init();
    virtual int Close();
    virtual int AsyncRead(
            void *buf, uint32_t len, uint64_t offset, io_completion_cb callback,
            void *ctx);
    virtual int AsyncWrite(
            const void *buf, uint32_t len, uint64_t offset,
            io_completion_cb callback, void *ctx);

    virtual int Read(void *buf, uint32_t len, uint64_t offset);
    virtual int Write(const void *buf, uint32_t len, uint64_t offset);

    virtual const std::string &GetDeviceId() const {
        return sopts_.blob_id;
    }
    virtual const std::string &GetDeviceName() const {
        return sopts_.blob_name;
    }
    virtual const std::string &GetUserId() const {
        return sopts_.user_id;
    }
    virtual const std::string &GetPoolId() const {
        return sopts_.pool_id;
    }
    virtual uint64_t GetExtentSize() const {
        return sopts_.extent_size;
    }
    virtual uint64_t GetDeviceSize() const {
        return sopts_.blob_size;
    }
    virtual uint64_t GetMaxIoSize() const {
        return sopts_.max_iosize;
    }
    virtual uint64_t GeOptimalIoSize() const {
        return sopts_.optimal_iosize;
    }
    virtual int64_t GetInflightIoNumber() const {
        return ioInflight_.load(std::memory_order_acquire);
    }

    virtual int SetExtentIoProto(ExtentIoProtocol esio);

private:
    RBDStreamHandleImpl(const RBDStreamHandleImpl &) = delete;
    RBDStreamHandleImpl operator=(const RBDStreamHandleImpl &) = delete;

    inline ExtentStreamHandlePtr GetExtentStreamHandle(uint64_t index);

    int splitUserRequest(
            const UserIoRequest *ureq, uint64_t roff[2], uint32_t len[2],
            ExtentStreamHandlePtr handle[2], int &size);

    int doUserReadRequest(UserReadRequest *ureq);
    int doUserWriteRequest(UserWriteRequest *ureq);

    void onReadDone(ReadRequest *req);
    void onWriteDone(WriteRequest *req);

    const RBDStreamOptions sopts_;
    std::unordered_map<uint64_t, ExtentStreamHandlePtr> handleMap_;
    // IoConcurrency concurrency_;
    ExtentIoProtocol esio_proto_;
    std::atomic<int64_t> ioInflight_;
    std::atomic<bool> isClosed_;
};

}  // namespace clients
}  // namespace cyprestore
#endif  // CYPRESTORE_CLIENTS_STREAM_RBD_STREAM_HANDLE_IMPL_H_
