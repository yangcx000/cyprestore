/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#ifndef RPCCLIENT_H_
#define RPCCLIENT_H_

#include <brpc/channel.h>
#include <time.h>

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include "common/pb/types.pb.h"

using namespace cyprestore::common::pb;
using namespace std;

typedef std::shared_ptr<SetRoute> SetRoutePtr;

namespace cyprestore {

class RpcClient {
public:
    RpcClient();
    virtual ~RpcClient();

    StatusCode Init(const std::string &server_addr, int port);

    string ServerIp;
    int ServerPort;
    brpc::Channel channel;

    StatusCode CheckResult(
            const string &RpcMethod, brpc::Controller *cntl,
            StatusCode statuscode, const string &statusmessage);

private:
};

string BlobTypeToString(BlobType blob_type);
string BlobStatusToString(BlobStatus blob_status);

}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENT_BASE_H_
