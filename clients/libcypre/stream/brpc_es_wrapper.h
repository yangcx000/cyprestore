
/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 */

#ifndef CYPRESTORE_CLIENTS_STREAM_CLUSTER_ES_WRAPPER_H_
#define CYPRESTORE_CLIENTS_STREAM_CLUSTER_ES_WRAPPER_H_

#include <brpc/channel.h>

#include <memory>
#include <vector>

#include "extentserver/pb/extent_io.pb.h"
#include "stream/es_wrapper.h"
#include "stream/extent_stream_handle.h"

namespace cyprestore {
namespace clients {

class BrpcSenderWorker;
class BrpcEsWrapper : public EsWrapper {
public:
    BrpcEsWrapper(
            const RBDStreamOptions &sopts, const ExtentStreamOptions &eopts)
            : sopts_(sopts), eopts_(eopts) {}

    virtual int AsyncRead(
            const common::ESInstance &es, ReadRequest *req,
            google::protobuf::Closure *done);
    virtual int AsyncWrite(
            const common::ESInstance &es, WriteRequest *req,
            google::protobuf::Closure *done);

    static int StartSenderWorker(
            int thread_size, int ring_size_power,
            const std::vector<int> &affinity, BrpcSenderWorker **sender);
    static void StopSenderWorker(BrpcSenderWorker *sender);

private:
    void onWriteDone(
            brpc::Controller *cntl, extentserver::pb::WriteResponse *resp,
            WriteRequest *req, google::protobuf::Closure *callback);

    const RBDStreamOptions &sopts_;
    const ExtentStreamOptions eopts_;
};

}  // namespace clients
}  // namespace cyprestore
#endif  // CYPRESTORE_CLIENTS_STREAM_ES_WRAPPER_H_
