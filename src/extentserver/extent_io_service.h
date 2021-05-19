/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_EXTENT_IO_SERVICE_H_
#define CYPRESTORE_EXTENTSERVER_EXTENT_IO_SERVICE_H_

#include <brpc/channel.h>

#include <string>

#include "pb/extent_io.pb.h"

namespace cyprestore {
namespace extentserver {

class ExtentIOServiceImpl : public pb::ExtentIOService {
public:
    ExtentIOServiceImpl() = default;
    virtual ~ExtentIOServiceImpl() = default;

    static void ReclaimIOUnit(void *arg);
    static void *ReadDone(void *arg);
    virtual void
    Read(google::protobuf::RpcController *cntl_base,
         const pb::ReadRequest *request, pb::ReadResponse *response,
         google::protobuf::Closure *done);

    static void *WriteDone(void *arg);
    virtual void
    Write(google::protobuf::RpcController *cntl_base,
          const pb::WriteRequest *request, pb::WriteResponse *response,
          google::protobuf::Closure *done);

    static void *DeleteDone(void *arg);
    virtual void
    Delete(google::protobuf::RpcController *cntl_base,
           const pb::DeleteRequest *request, pb::DeleteResponse *response,
           google::protobuf::Closure *done);

    static void *ReplicateDone(void *arg);
    virtual void Replicate(
            google::protobuf::RpcController *cntl_base,
            const pb::ReplicateRequest *request,
            pb::ReplicateResponse *response, google::protobuf::Closure *done);

    static void *ScrubDone(void *arg);
    virtual void
    Scrub(google::protobuf::RpcController *cntl_base,
          const pb::ScrubRequest *request, pb::ScrubResponse *response,
          google::protobuf::Closure *done);
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_EXTENT_IO_SERVICE_H_
