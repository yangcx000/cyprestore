/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_SETMANAGER_HEARTBEAT_SERVICE_H_
#define CYPRESTORE_SETMANAGER_HEARTBEAT_SERVICE_H_

#include "common/pb/types.pb.h"
#include "setmanager/pb/heartbeat.pb.h"

namespace cyprestore {
namespace setmanager {

class HeartbeatServiceImpl : public pb::HeartbeatService {
public:
    virtual void ReportHeartbeat(
            google::protobuf::RpcController *cntl_base,
            const pb::HeartbeatRequest *request,
            pb::HeartbeatResponse *response, google::protobuf::Closure *done);

    void PrintSet(const common::pb::Set &set);
};

}  // namespace setmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_SETMANAGER_HEARTBEAT_SERVICE_H_
