
/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 */

#include "stream/rbd_stream_handle_impl.h"

#include <brpc/callback.h>

#include <memory>

#include "common/builtin.h"
#include "common/error_code.h"
#include "common/extent_id_generator.h"
#include "stream/ystream_handle.h"
#include "utils/crc32.h"

using namespace cyprestore;
using namespace cyprestore::clients;

RBDStreamHandleImpl::RBDStreamHandleImpl(const RBDStreamOptions &opt)
        : RBDStreamHandle(), sopts_(opt), esio_proto_(kBrpc), ioInflight_(0),
          isClosed_(false) {}

RBDStreamHandleImpl::~RBDStreamHandleImpl() {
    Close();
}

int RBDStreamHandleImpl::Init() {
    return common::CYPRE_OK;
}

int RBDStreamHandleImpl::Close() {
    int rv = common::CYPRE_OK;
    if (isClosed_.exchange(true)) {
        return rv;
    }
    int64_t ion = ioInflight_.load(std::memory_order_acquire);
    LOG(WARNING) << "Close Handle: " << this << ", Blob:" << sopts_.blob_id
                 << ", name:" << sopts_.blob_name
                 << ", Current inflight Io number:" << ion;
    while (ioInflight_.load(std::memory_order_acquire) != 0) {
        usleep(1);
    }
    for (auto itr = handleMap_.begin(); itr != handleMap_.end(); ++itr) {
        ExtentStreamHandlePtr handle = itr->second;
        int rc = handle->Close();
        if (rc != common::CYPRE_OK) {
            rv = rc;
            LOG(ERROR) << "Close ExtentSream Failed, rv:" << rc
                       << ", Extent:" << handle->GetExtentId();
        }
    }
    handleMap_.clear();
    return rv;
}

ExtentStreamHandlePtr
RBDStreamHandleImpl::GetExtentStreamHandle(uint64_t extent_index) {
    auto itr = handleMap_.find(extent_index);
    if (itr != handleMap_.end()) {
        return itr->second;
    }
    ExtentStreamOptions exopt;
    exopt.extent_id = common::ExtentIDGenerator::GenerateExtentID(
            sopts_.blob_id, extent_index);
    exopt.extent_idx = extent_index;
    ExtentStreamHandlePtr handle(new YStreamHandle(sopts_, exopt));
    int rv = handle->Init();
    if (rv != common::CYPRE_OK) {
        LOG(ERROR) << "Init ExtentSream Failed, rv:" << rv
                   << ", Extent:" << handle->GetExtentId();
        handle.reset();
    } else {
        handleMap_[extent_index] = handle;
    }
    handle->SetExtentIoProto(esio_proto_);
    return handle;
}

int RBDStreamHandleImpl::Read(void *buf, uint32_t len, uint64_t offset) {
    return AsyncRead(buf, len, offset, NULL, NULL);
}

int RBDStreamHandleImpl::Write(const void *buf, uint32_t len, uint64_t offset) {
    return AsyncWrite(buf, len, offset, NULL, NULL);
}

int RBDStreamHandleImpl::AsyncRead(
        void *buf, uint32_t len, uint64_t offset, io_completion_cb callback,
        void *ctx) {
    if (isClosed_.load(std::memory_order_relaxed)) {
        return common::CYPRE_C_DEVICE_CLOSED;
    }
    // TODO(zhangliang): use pool
    UserReadRequest *ureq = new UserReadRequest();
    ureq->buf = buf;
    ureq->logic_len = len;
    ureq->logic_offset = offset;
    ureq->user_cb = callback;
    ureq->user_ctx = ctx;
    ureq->generateHeaderCrc32();
    return doUserReadRequest(ureq);
}

int RBDStreamHandleImpl::AsyncWrite(
        const void *buf, uint32_t len, uint64_t offset,
        io_completion_cb callback, void *ctx) {
    if (isClosed_.load(std::memory_order_relaxed)) {
        return common::CYPRE_C_DEVICE_CLOSED;
    }
    // TODO(zhangliang): use pool
    UserWriteRequest *ureq = new UserWriteRequest();
    ureq->buf = buf;
    ureq->logic_len = len;
    ureq->logic_offset = offset;
    ureq->user_cb = callback;
    ureq->user_ctx = ctx;
    ureq->data_crc32_ = utils::Crc32::Checksum(buf, len);
    ureq->generateHeaderCrc32();
    return doUserWriteRequest(ureq);
}

int RBDStreamHandleImpl::splitUserRequest(
        const UserIoRequest *ureq, uint64_t roff[2], uint32_t len[2],
        ExtentStreamHandlePtr handle[2], int &size) {
    const uint64_t extSize = sopts_.extent_size;
    uint64_t extent_index1 = ureq->logic_offset / extSize + 1;
    uint64_t extent_index2 =
            (ureq->logic_offset + ureq->logic_len - 1) / extSize + 1;
    if (extent_index2 > extent_index1 + 1) {
        return common::CYPRE_C_DATA_TOO_LARGE;
    }
    size = (extent_index2 > extent_index1 ? 2 : 1);
    handle[0] = GetExtentStreamHandle(extent_index1);
    if (handle[0].get() == nullptr) {
        return common::CYPRE_C_INTERNAL_ERROR;
    }
    roff[0] = ureq->logic_offset - (extent_index1 - 1) * extSize;
    if (size == 2) {
        len[0] = extSize - roff[0];
        handle[1] = GetExtentStreamHandle(extent_index2);
        if (handle[1].get() == nullptr) {
            return common::CYPRE_C_INTERNAL_ERROR;
        }
        roff[1] = 0;
        len[1] = ureq->logic_len - len[0];
    } else {
        len[0] = ureq->logic_len;
    }
    return common::CYPRE_OK;
}

int RBDStreamHandleImpl::doUserReadRequest(UserReadRequest *ureq) {
    int ionum = 1;
    uint64_t roff[2] = { 0, 0 };
    uint32_t len[2] = { 0, 0 };
    ExtentStreamHandlePtr handle[2];
    int rv = splitUserRequest(ureq, roff, len, handle, ionum);
    if (rv != common::CYPRE_OK) {
        LOG(ERROR) << "splitUserRequest Failed, rv=" << rv
                   << ", Blob:" << sopts_.blob_id
                   << ", offset:" << ureq->logic_offset
                   << ", len:" << ureq->logic_len;
        delete ureq;
        return rv;
    }
    ioInflight_.fetch_add(1, std::memory_order_release);
    ureq->ref = ionum;
    if (ionum == 2) {
        ureq->is_splited_ = true;
    }
    void *buf = ureq->buf;
    for (int i = 0; i < ionum; i++) {
        // TODO(zhangliang): use pool
        ReadRequest *req = new ReadRequest();
        req->handle = handle[i];
        req->buf = buf;
        req->real_len = len[i];
        req->real_offset = roff[i];
        req->ureq = ureq;
        req->header_crc32_ = ureq->header_crc32_;
        buf = (char *)buf + len[i];
        google::protobuf::Closure *cb =
                brpc::NewCallback(this, &RBDStreamHandleImpl::onReadDone, req);
        if (likely(ureq->user_cb != NULL)) {
            int rc = handle[i]->AsyncRead(req, cb);
            if (rc != common::CYPRE_OK) {
                rv = rc;
                cb->Run();
            }
        } else {  // sync mode
            int rc = handle[i]->AsyncRead(req, NULL);
            if (rc != common::CYPRE_OK) {
                rv = rc;
            }
            cb->Run();
        }
    }
    return rv;
}

int RBDStreamHandleImpl::doUserWriteRequest(UserWriteRequest *ureq) {
    int ionum = 1;
    uint64_t roff[2] = { 0, 0 };
    uint32_t len[2] = { 0, 0 };
    ExtentStreamHandlePtr handle[2];
    int rv = splitUserRequest(ureq, roff, len, handle, ionum);
    if (rv != common::CYPRE_OK) {
        LOG(ERROR) << "splitUserRequest Failed, rv=" << rv
                   << ", Blob:" << sopts_.blob_id
                   << ", offset:" << ureq->logic_offset
                   << ", len:" << ureq->logic_len;
        delete ureq;
        return rv;
    }
    ioInflight_.fetch_add(1, std::memory_order_release);
    ureq->ref = ionum;
    if (ionum == 2) {
        ureq->is_splited_ = true;
    }
    const void *buf = ureq->buf;
    for (int i = 0; i < ionum; i++) {
        // TODO(zhangliang): use pool
        WriteRequest *req = new WriteRequest();
        req->handle = handle[i];
        req->buf = buf;
        req->real_len = len[i];
        req->real_offset = roff[i];
        req->ureq = ureq;
        req->header_crc32_ = ureq->header_crc32_;
        if (ureq->is_splited_) {
            req->data_crc32_ = utils::Crc32::Checksum(req->buf, req->real_len);
        } else {
            req->data_crc32_ = ureq->data_crc32_;
        }
        buf = (const char *)buf + len[i];
        google::protobuf::Closure *cb =
                brpc::NewCallback(this, &RBDStreamHandleImpl::onWriteDone, req);
        if (likely(ureq->user_cb != NULL)) {
            int rc = handle[i]->AsyncWrite(req, cb);
            if (rc != common::CYPRE_OK) {
                rv = rc;
                cb->Run();
            }
        } else {  // sync mode
            int rc = handle[i]->AsyncWrite(req, NULL);
            if (rc != common::CYPRE_OK) {
                rv = rc;
            }
            cb->Run();
        }
    }
    return rv;
}

void RBDStreamHandleImpl::onReadDone(ReadRequest *req) {
    UserReadRequest *ureq = req->ureq;
    if (req->status == common::CYPRE_OK  && req->has_data_crc32_) {
        uint32_t crc32 = utils::Crc32::Checksum(req->buf, req->real_len);
        if (req->data_crc32_ != crc32) {
            LOG(ERROR) << "onReadDone crc32 check error, c3c32 of read buffer: " << crc32
                       << ", crc32 in response: " << req->data_crc32_
                       << ", Blob: " << sopts_.blob_id
                       << ", offset: " << req->real_offset
                       << ", len: " << req->real_len
                       << ", splited request: " << ureq->is_splited_;
            ureq->status = common::CYPRE_C_CRC_ERROR;
        }
    }
    if (req->status != common::CYPRE_OK && ureq->status == common::CYPRE_OK) {
        ureq->status = req->status;
    }
    delete req;
    if (--ureq->ref > 0) {
        return;
    }
    if (ureq->user_cb) {
        ureq->user_cb(ureq->status, ureq->user_ctx);
    }
    ioInflight_.fetch_sub(1, std::memory_order_release);
    delete ureq;
}

void RBDStreamHandleImpl::onWriteDone(WriteRequest *req) {
    UserWriteRequest *ureq = req->ureq;
    if (req->status != common::CYPRE_OK && ureq->status == common::CYPRE_OK) {
        ureq->status = req->status;
    }
    delete req;
    if (--ureq->ref > 0) {
        return;
    }
    if (ureq->user_cb) {
        ureq->user_cb(ureq->status, ureq->user_ctx);
    }
    ioInflight_.fetch_sub(1, std::memory_order_release);
    delete ureq;
}

int RBDStreamHandleImpl::SetExtentIoProto(ExtentIoProtocol esio) {
    if (esio_proto_ == esio) {
        return common::CYPRE_OK;
    }
    int rv = common::CYPRE_OK;
    esio_proto_ = esio;
    for (auto itr = handleMap_.begin(); itr != handleMap_.end(); ++itr) {
        ExtentStreamHandlePtr handle = itr->second;
        int rc = handle->SetExtentIoProto(esio);
        if (rc != common::CYPRE_OK) {
            rv = rc;
            LOG(ERROR) << "Switch ExtentSream Failed, rv:" << rc
                       << ", Extent:" << handle->GetExtentId();
        }
    }
    return rv;
}
