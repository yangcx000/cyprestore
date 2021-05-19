
/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 */

#ifndef CYPRESTORE_CLIENTS_STREAM_STREAM_HANDLE_H_
#define CYPRESTORE_CLIENTS_STREAM_STREAM_HANDLE_H_

#include <time.h>

#include <memory>

#include "common/connection_pool.h"
#include "common/extent_router.h"
#include "libcypre_common.h"
#include "stream/user_request.h"

namespace cyprestore {
namespace clients {

class RBDStreamOptions;
class RBDStreamHandle;
class ExtentStreamHandle;
typedef std::shared_ptr<ExtentStreamHandle> ExtentStreamHandlePtr;

struct ExtentStreamOptions {
    ExtentStreamOptions() : extent_idx(0) {}
    std::string extent_id;
    int extent_idx;
};

struct ReadRequest {
    ReadRequest() : buf(NULL), data_crc32_(0), has_data_crc32_(false),
            header_crc32_(0), ureq(NULL), is_done(false), status(-1) {}
    void *buf;
    uint32_t data_crc32_;
    bool has_data_crc32_;
    uint32_t header_crc32_;
    // range in extent
    uint32_t real_offset;
    uint32_t real_len;
    ExtentStreamHandlePtr handle;
    UserReadRequest *ureq;
    std::atomic<bool> is_done;
    int status;
};

struct WriteRequest {
    WriteRequest() : buf(NULL), data_crc32_(0), header_crc32_(0),
            ureq(NULL), is_done(false), status(-1) {}
    const void *buf;
    uint32_t data_crc32_;
    uint32_t header_crc32_;
    // range in extent
    uint32_t real_offset;
    uint32_t real_len;
    ExtentStreamHandlePtr handle;
    UserWriteRequest *ureq;
    std::atomic<bool> is_done;
    int status;
};

class ExtentStreamHandle {
public:
    ExtentStreamHandle(
            const RBDStreamOptions &sopt, const ExtentStreamOptions &eopt)
            : sopts_(sopt), eopts_(eopt) {}
    virtual ~ExtentStreamHandle() {}

    virtual int Init() = 0;
    virtual int Close() = 0;
    virtual int
    AsyncRead(ReadRequest *req, google::protobuf::Closure *done) = 0;
    virtual int
    AsyncWrite(WriteRequest *req, google::protobuf::Closure *done) = 0;

    virtual int SetExtentIoProto(ExtentIoProtocol esio) = 0;
    inline const std::string &GetExtentId() const {
        return eopts_.extent_id;
    }
    inline int GetExtentIndex() const {
        return eopts_.extent_idx;
    }

protected:
    const RBDStreamOptions &sopts_;
    const ExtentStreamOptions eopts_;
};

}  // namespace clients
}  // namespace cyprestore
#endif  // CYPRESTORE_CLIENTS_STREAM_STREAM_HANDLE_H_
