/*
 * Copyright 2020 JDD authors.
 * @yfeifei5
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_EXTENT_CONTROL_SERVICE_H_
#define CYPRESTORE_EXTENTSERVER_EXTENT_CONTROL_SERVICE_H_

#include <brpc/channel.h>

#include <string>

#include "pb/extent_control.pb.h"

namespace cyprestore {
namespace extentserver {

class ExtentControlServiceImpl : public pb::ExtentControlService {
public:
    ExtentControlServiceImpl() = default;
    virtual ~ExtentControlServiceImpl() = default;

    virtual void ReclaimExtent(
            google::protobuf::RpcController *cntl_base,
            const pb::ReclaimExtentRequest *request,
            pb::ReclaimExtentResponse *response,
            google::protobuf::Closure *done);
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_EXTENT_CONTROL_SERVICE_H_
