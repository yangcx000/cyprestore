/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#ifndef RPCCLIENT_SM_H_
#define RPCCLIENT_SM_H_

#include "common/pb/types.pb.h"
#include "rpcclient_base.h"

namespace cyprestore {

class RpcClientSM : public RpcClient {
public:
    RpcClientSM();
    virtual ~RpcClientSM();

    StatusCode List(std::list<SetRoutePtr> &sets, std::string &errmsg);
    StatusCode Allocate(
            const string &user_name, PoolType pool_type, SetRoute &set,
            std::string &errmsg);
    StatusCode
    Query(const string &user_id, SetRoute &outset, std::string &errmsg);
    StatusCode ReportHeartbeat(
            const cyprestore::common::pb::Set &set, std::string &errmsg);
};

}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENT_SM_H_
