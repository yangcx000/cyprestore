/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_SETMANAGER_SET_SERVICE_H_
#define CYPRESTORE_SETMANAGER_SET_SERVICE_H_

#include <brpc/channel.h>

#include "setmanager/pb/set.pb.h"
#include "setmanager/setmanager.h"

namespace cyprestore {
namespace setmanager {

class SetServiceImpl : public pb::SetService {
public:
    virtual void
    List(google::protobuf::RpcController *cntl_base,
         const pb::ListSetsRequest *request, pb::ListSetsResponse *response,
         google::protobuf::Closure *done);

    virtual void Allocate(
            google::protobuf::RpcController *cntl_base,
            const pb::AllocateSetRequest *request,
            pb::AllocateSetResponse *response, google::protobuf::Closure *done);

    virtual void
    Query(google::protobuf::RpcController *cntl_base,
          const pb::QuerySetRequest *request, pb::QuerySetResponse *response,
          google::protobuf::Closure *done);

private:
    int BroadcastRequest(const SetArray &sets, const std::string &user_id);
};

}  // namespace setmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_SETMANAGER_SET_SERVICE_H_
