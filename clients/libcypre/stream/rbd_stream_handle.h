
/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 */

#ifndef CYPRESTORE_CLIENTS_STREAM_RBD_STREAM_HANDLE_H_
#define CYPRESTORE_CLIENTS_STREAM_RBD_STREAM_HANDLE_H_

#include <memory>
#include <string>

#include "libcypre_common.h"

namespace cyprestore {
namespace clients {

typedef void (*io_completion_cb)(int rc, void *ctx);

class RBDStreamHandle {
public:
    RBDStreamHandle() {}
    virtual ~RBDStreamHandle() {}

    virtual int Init() = 0;
    virtual int Close() = 0;
    // return CYPREC_OK is success
    // @callback, for sync mode, set to NULL
    virtual int AsyncRead(
            void *buf, uint32_t len, uint64_t offset, io_completion_cb callback,
            void *ctx) = 0;
    // return CYPREC_OK is success
    // @callback, for sync mode, set to NULL
    virtual int AsyncWrite(
            const void *buf, uint32_t len, uint64_t offset,
            io_completion_cb callback, void *ctx) = 0;

    virtual int Read(void *buf, uint32_t len, uint64_t offset) = 0;
    virtual int Write(const void *buf, uint32_t len, uint64_t offset) = 0;

    virtual const std::string &GetDeviceId() const = 0;
    virtual const std::string &GetDeviceName() const = 0;
    virtual const std::string &GetUserId() const = 0;
    virtual const std::string &GetPoolId() const = 0;
    virtual uint64_t GetExtentSize() const = 0;    // in bytes
    virtual uint64_t GetDeviceSize() const = 0;    // in bytes
    virtual uint64_t GetMaxIoSize() const = 0;     // in bytes
    virtual uint64_t GeOptimalIoSize() const = 0;  // in bytes
    virtual int64_t GetInflightIoNumber() const = 0;

    virtual int SetExtentIoProto(ExtentIoProtocol esio) = 0;
};
typedef std::shared_ptr<RBDStreamHandle> RBDStreamHandlePtr;

}  // namespace clients
}  // namespace cyprestore
#endif  // CYPRESTORE_CLIENTS_STREAM_RBD_STREAM_HANDLE_H_
