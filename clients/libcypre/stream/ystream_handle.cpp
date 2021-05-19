
/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 */

#include "stream/ystream_handle.h"

#include <brpc/channel.h>

#include <memory>

#include "common/builtin.h"
#include "common/error_code.h"
#include "extentmanager/pb/resource.pb.h"
#include "stream/brpc_es_wrapper.h"
#include "stream/rbd_stream_handle_impl.h"

namespace cyprestore {
namespace clients {

YStreamHandle::YStreamHandle(
        const RBDStreamOptions &sopt, const ExtentStreamOptions &eopt)
        : ExtentStreamHandle(sopt, eopt) {
    brpc_es_wrapper_ = new BrpcEsWrapper(sopt, eopt);
    null_es_wrapper_ = new NullEsWrapper(sopt, eopt);
    es_wrapper_ = brpc_es_wrapper_;
}

int YStreamHandle::Init() {
    return common::CYPRE_OK;
}

int YStreamHandle::Close() {
    return common::CYPRE_OK;
}

int YStreamHandle::AsyncRead(
        ReadRequest *req, google::protobuf::Closure *callback) {
    // get rpc channel
    common::ExtentRouterPtr router =
            sopts_.extent_router_mgr->QueryRouter(eopts_.extent_id);
    if (unlikely(!router)) {
        LOG(ERROR) << "Couldn't get extent router"
                   << ", extent_id:" << eopts_.extent_id;
        req->is_done = true;
        req->status = common::CYPRE_C_INTERNAL_ERROR;
        return req->status;
    }
    return es_wrapper_->AsyncRead(router->primary, req, callback);
}

int YStreamHandle::AsyncWrite(
        WriteRequest *req, google::protobuf::Closure *callback) {
    // get rpc channel
    common::ExtentRouterPtr router =
            sopts_.extent_router_mgr->QueryRouter(eopts_.extent_id);
    if (unlikely(!router)) {
        LOG(ERROR) << "Couldn't get extent router"
                   << ", extent_id:" << eopts_.extent_id;
        req->is_done = true;
        req->status = common::CYPRE_C_INTERNAL_ERROR;
        return req->status;
    }
    return es_wrapper_->AsyncWrite(router->primary, req, callback);
}

int YStreamHandle::SetExtentIoProto(ExtentIoProtocol esio) {
    if (esio == ExtentIoProtocol::kNull) {
        es_wrapper_ = null_es_wrapper_;
    } else if (esio == ExtentIoProtocol::kBrpc) {
        es_wrapper_ = brpc_es_wrapper_;
    } else {
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }
    return common::CYPRE_OK;
}

}  // namespace clients
}  // namespace cyprestore
