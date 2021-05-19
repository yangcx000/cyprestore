
/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 */

#ifndef CYPRESTORE_CLIENTS_STREAM_CLUSTER_STREAM_HANDLE_H_
#define CYPRESTORE_CLIENTS_STREAM_CLUSTER_STREAM_HANDLE_H_

#include <brpc/channel.h>

#include <memory>

#include "stream/es_wrapper.h"
#include "stream/extent_stream_handle.h"

namespace cyprestore {

namespace extentserver {
namespace pb {
class ReadResponse;
class WriteResponse;
}  // namespace pb
}  // namespace extentserver

namespace clients {

class YStreamHandle : public ExtentStreamHandle {
public:
    YStreamHandle(
            const RBDStreamOptions &sopt, const ExtentStreamOptions &eopt);
    virtual ~YStreamHandle() {}

    virtual int Init();
    virtual int Close();
    virtual int
    AsyncRead(ReadRequest *req, google::protobuf::Closure *callback);
    virtual int
    AsyncWrite(WriteRequest *req, google::protobuf::Closure *callback);
    virtual int SetExtentIoProto(ExtentIoProtocol esio);

private:
    EsWrapper *es_wrapper_;
    EsWrapper *brpc_es_wrapper_;
    EsWrapper *null_es_wrapper_;
};

}  // namespace clients
}  // namespace cyprestore
#endif  // CYPRESTORE_CLIENTS_STREAM_CLUSTER_STREAM_HANDLE_H_
