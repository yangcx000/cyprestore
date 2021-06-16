/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTSERVER_REQUEST_CONTEXT_H
#define CYPRESTORE_EXTENTSERVER_REQUEST_CONTEXT_H

#include <brpc/channel.h>
#include <butil/macros.h>
#include <google/protobuf/message.h>

#include <atomic>
#include <memory>

#include "common/config.h"
#include "common/ctx_mem.h"
#include "common/extent_router.h"
#include "io_mem.h"
#include "utils/chrono.h"

namespace cyprestore {
namespace extentserver {

typedef void *(*UserCallback_t)(void *arg);

class RequestMgr;
typedef std::shared_ptr<RequestMgr> RequestMgrPtr;

enum RequestType {
    kTypeRead = 0,
    kTypeWrite,
    kTypeReplicate,
    kTypeScrub,
    kTypeDelete,
    kTypeReclaimExtent,
    kTypeNoop = -1,
};

struct OperationContext {
    OperationContext()
            : cntl(nullptr), request(nullptr), response(nullptr),
              done(nullptr) {}

    brpc::Controller *cntl;
    google::protobuf::Message *request;
    google::protobuf::Message *response;
    google::protobuf::Closure *done;
};

class Request {
public:
    Request(RequestType request_type)
            : result_(true), ref_count_(1), request_type_(request_type),
              user_cb_(nullptr), io_unit_(nullptr), iomem_mgr_(nullptr),
              extent_router_(nullptr), physical_offset_(0), crc32_(0) {
        req_begin_ = {0, 0};
        req_end_ = {0, 0};
    }
    ~Request() = default;

    void Reset(RequestType request_type) {
        result_ = true;
        ref_count_ = 1;
        request_type_ = request_type;
        user_cb_ = nullptr;
        io_unit_ = nullptr;
        iomem_mgr_ = nullptr;
        extent_router_ = nullptr;
        physical_offset_ = 0;
        crc32_ = 0;
        req_begin_ = {0, 0};
        req_end_ = {0, 0};
    }

    bool Result() const {
        return result_;
    }
    void SetResult(bool success) {
        result_ = result_ && success;
    }

    int RefCount() const {
        return ref_count_.load();
    }
    int FetchAndSubRef() {
        return ref_count_.fetch_sub(1);
    }
    int FetchAndAddRef() {
        return ref_count_.fetch_add(1);
    }
    void SetRefCount(int ref_count) {
        ref_count_.store(ref_count);
    }

    RequestType GetRequestType() const {
        return request_type_;
    }

    UserCallback_t UserCallback() const {
        return user_cb_;
    }
    void SetUserCallback(UserCallback_t user_cb) {
        user_cb_ = user_cb;
    }

    OperationContext &GetOperationContext() {
        return op_ctx_;
    }
    void SetOperationContext(
            brpc::Controller *cntl, google::protobuf::Message *request,
            google::protobuf::Message *response,
            google::protobuf::Closure *done);

    std::string ExtentID() const;
    uint64_t Offset() const;
    uint64_t Size() const;

    io_u *IOUnit() {
        return io_unit_;
    }
    void SetIOUnit(io_u *io) {
        io_unit_ = io;
    }

    std::shared_ptr<IOMemMgr> GetIOMemMgr() const {
        return iomem_mgr_;
    }
    void SetIOMemMgr(std::shared_ptr<IOMemMgr> iomem_mgr) {
        iomem_mgr_ = iomem_mgr;
    }

    void SetExtentRouter(common::ExtentRouterPtr extent_router) {
        extent_router_ = extent_router;
    }
    common::ExtentRouterPtr GetExtentRouter() const {
        return extent_router_;
    }

    void SetEmptyResponse() const {
        op_ctx_.cntl->response_attachment().resize(Size());
    }

    uint64_t PhysicalOffset() const {
        return physical_offset_;
    }
    void SetPhysicalOffset(uint64_t physical_offset) {
        physical_offset_ = physical_offset;
    }

    uint32_t Crc32() const {
        return crc32_;
    }
    void SetCrc32(uint32_t crc32) {
        crc32_ = crc32;
    }

    void BeginTraceTime() { utils::Chrono::GetTime(&req_begin_); }
    void EndTraceTime() {
        utils::Chrono::GetTime(&req_end_);
        auto time_elapsed_us = utils::Chrono::TimeSinceUs(&req_begin_, &req_end_);
        auto timeout = GlobalConfig().extentserver().slow_request_time * 1000;
        if (time_elapsed_us > static_cast<uint64_t>(timeout)) {
            LOG(ERROR) << "Process request cost time: " << time_elapsed_us
                       << " us, request type: " << request_type_
                       << ", extent id: " << ExtentID()
                       << ", physical offset: " << physical_offset_
                       << ", logical offset: " << Offset()
                       << ", size: " << Size();
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(Request);

    bool result_;
    std::atomic<int> ref_count_;
    RequestType request_type_;
    UserCallback_t user_cb_;
    OperationContext op_ctx_;

    io_u *io_unit_;
    std::shared_ptr<IOMemMgr> iomem_mgr_;
    common::ExtentRouterPtr extent_router_;

    uint64_t physical_offset_;
    uint32_t crc32_;

    struct timespec req_begin_;
    struct timespec req_end_;
};

class RequestMgr {
public:
    RequestMgr() = default;
    ~RequestMgr() = default;

    Request *GetRequest(RequestType request_type);
    void PutRequest(Request *req);

    int Init();

private:
    std::unique_ptr<common::CtxMemMgr<Request>> ctxmem_mgr_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_REQUEST_CONTEXT_H
