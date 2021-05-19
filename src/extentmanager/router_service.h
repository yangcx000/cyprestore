/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTMANAGER_ROUTER_SERVICE_H_
#define CYPRESTORE_EXTENTMANAGER_ROUTER_SERVICE_H_

#include "extentmanager/pb/router.pb.h"

namespace cyprestore {
namespace extentmanager {

class RouterServiceImpl : public pb::RouterService {
public:
    RouterServiceImpl() = default;
    virtual ~RouterServiceImpl() = default;

    virtual void QueryRouter(
            google::protobuf::RpcController *cntl_base,
            const pb::QueryRouterRequest *request,
            pb::QueryRouterResponse *response, google::protobuf::Closure *done);
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_ROUTER_SERVICE_H_
