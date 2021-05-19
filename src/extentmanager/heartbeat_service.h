/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_HEARTBEAT_SERVICE_H_
#define CYPRESTORE_EXTENTMANAGER_HEARTBEAT_SERVICE_H_

#include "common/pb/types.pb.h"
#include "extentmanager/pb/heartbeat.pb.h"

namespace cyprestore {
namespace extentmanager {

class HeartbeatServiceImpl : public pb::HeartbeatService {
public:
    HeartbeatServiceImpl() = default;
    virtual ~HeartbeatServiceImpl() = default;

    virtual void ReportHeartbeat(
            google::protobuf::RpcController *cntl_base,
            const pb::ReportHeartbeatRequest *request,
            pb::ReportHeartbeatResponse *response,
            google::protobuf::Closure *done);

private:
    void DescribeExtentServer(const common::pb::ExtentServer &es);
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_HEARTBEAT_SERVICE_H_
