/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
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
