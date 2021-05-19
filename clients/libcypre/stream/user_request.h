
/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 */

#ifndef CYPRESTORE_CLIENTS_STREAM_USER_REQUEST_H_
#define CYPRESTORE_CLIENTS_STREAM_USER_REQUEST_H_

#include <atomic>
#include <memory>
#include <string>

#include "common/error_code.h"
#include "stream/rbd_stream_handle.h"
#include "utils/crc32.h"

namespace cyprestore {
namespace clients {

enum StreamIoType { kRead = 0, kWrite, kInvalid };

class UserRequest {
public:
    UserRequest(const StreamIoType &t)
            : is_splited_(false), user_cb(NULL), user_ctx(NULL),
              status(common::CYPRE_OK), ref(0), type(t) {}
    virtual ~UserRequest() {}
    // get io type
    StreamIoType GetIoType() const {
        return type;
    }

    bool is_splited_;
    io_completion_cb user_cb;
    void *user_ctx;
    int status;  // request status
    std::atomic<int> ref;

private:
    const StreamIoType type;
};

class UserIoRequest : public UserRequest {
public:
    UserIoRequest(const StreamIoType &t)
            : UserRequest(t), header_crc32_(0), logic_len(0), logic_offset(0) {}
    void generateHeaderCrc32() {
        std::string header = std::to_string(logic_len);
        header.append(std::to_string(logic_offset));
        header_crc32_ = utils::Crc32::Checksum(header);
    }

    uint32_t header_crc32_;
    uint32_t logic_len;
    uint64_t logic_offset;
};

class UserWriteRequest : public UserIoRequest {
public:
    UserWriteRequest() : UserIoRequest(kWrite), buf(NULL), data_crc32_(0) {}
    const void *buf;
    uint32_t data_crc32_;
};

class UserReadRequest : public UserIoRequest {
public:
    UserReadRequest() : UserIoRequest(kRead), buf(NULL) {}
    void *buf;
};

}  // namespace clients
}  // namespace cyprestore
#endif  // CYPRESTORE_CLIENTS_STREAM_USER_REQUEST_H_
