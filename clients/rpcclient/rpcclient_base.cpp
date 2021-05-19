/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#include "rpcclient_base.h"

#include <brpc/channel.h>

namespace cyprestore {

RpcClient::RpcClient() {}

RpcClient::~RpcClient() {}

StatusCode RpcClient::Init(const std::string &server_addr, int port) {
    ServerIp = server_addr;
    ServerPort = port;

    brpc::ChannelOptions options;
    options.protocol = "baidu_std";
    options.connection_type =
            "single";  // // Possible values: "single", "pooled", "short".
    options.timeout_ms = 20000; /*milliseconds*/
    ;
    options.max_retry = 3;  // Default: 3
    if (channel.Init(server_addr.c_str(), port, &options) != 0) {
        LOG(ERROR) << "Fail to initialize channel " << server_addr + ":"
                   << port;
        return (STATUS_FAILED_PRECONDITION);
    }

    return (STATUS_OK);
}

StatusCode RpcClient::CheckResult(
        const string &RpcMethod, brpc::Controller *cntl, StatusCode statuscode,
        const string &statusmessage) {
    if (cntl->Failed()) {
        LOG(ERROR) << RpcMethod + " call failed " << cntl->ErrorText();
        return (STATUS_INTERNAL);
    } else if (statuscode != STATUS_OK) {
        LOG(WARNING) << RpcMethod + " failed status=" << statuscode
                     << "," + statusmessage;
        return (statuscode);
    }

    string AttachInfo;
    if (cntl->response_attachment().length() > 0)
        AttachInfo = " (attached="
                     + to_string(cntl->response_attachment().length()) + ")";

    LOG(INFO) << RpcMethod + " response from " << cntl->remote_side() << " to "
              << cntl->local_side() << " latency=" << cntl->latency_us()
              << "us " + AttachInfo;
    return (STATUS_OK);
}

string BlobTypeToString(BlobType blob_type) {
    string typestr;
    switch (blob_type) {
        case BLOB_TYPE_GENERAL:
            typestr = "GENERAL";
            break;
        case BLOB_TYPE_SUPERIOR:
            typestr = "SUPERIOR ";
            break;
        case BLOB_TYPE_EXCELLENT:
            typestr = "EXCELLENT";
            break;
        case BLOB_TYPE_UNKNOWN:
            typestr = "UNKNOWN";
            break;
    };
    return (typestr);
}

string BlobStatusToString(BlobStatus blob_status) {
    string str;
    switch (blob_status) {
        case BLOB_STATUS_CREATED:
            str = "CREATED";
            break;
        case BLOB_STATUS_DELETED:
            str = "DELETED ";
            break;
        case BLOB_STATUS_RESIZING:
            str = "RESIZING";
            break;
        case BLOB_STATUS_SNAPSHOTTING:
            str = "SNAPSHOTTING ";
            break;
        case BLOB_STATUS_CLONING:
            str = "CLONING";
            break;
        case BLOB_STATUS_UNKNOWN:
            str = "UNKNOWN";
            break;
    };
    return (str);
}

}  // namespace cyprestore
