/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */
#include "rpcclient_es.h"

#include "extentserver/pb/extent.pb.h"

namespace cyprestore {

RpcClientES::RpcClientES() {}

RpcClientES::~RpcClientES() {}

StatusCode RpcClientES::Read(
        const std::string &extent_id, uint64_t offset, void *buf,
        uint32_t bufsize, uint32_t &retreadlen, std::string &crc32,
        std::string &errmsg) {
    cyprestore::extentserver::pb::ReadRequest request;
    cyprestore::extentserver::pb::ReadResponse response;
    request.set_extent_id(extent_id);
    request.set_offset(offset);
    request.set_size(bufsize);

    cyprestore::extentserver::pb::ExtentService_Stub stub(&channel);
    brpc::Controller cntl;
    stub.Read(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientES.Read", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    crc32 = response.crc32();
    retreadlen = cntl.response_attachment().copy_to(buf, bufsize);
    return (STATUS_OK);
}

// Write
StatusCode RpcClientES::Write(
        const std::string &extent_id, uint64_t offset, const void *buf,
        uint32_t buflen, const std::string &crc32, std::string &errmsg) {
    cyprestore::extentserver::pb::WriteRequest request;
    cyprestore::extentserver::pb::WriteResponse response;
    request.set_extent_id(extent_id);
    request.set_offset(offset);
    request.set_size(buflen);
    request.set_crc32(crc32);

    cyprestore::extentserver::pb::ExtentService_Stub stub(&channel);
    brpc::Controller cntl;
    if (cntl.request_attachment().append(buf, buflen) == -1) {
        LOG(WARNING) << "RpcClientES.Write request_attachment().append failed "
                     << cntl.ErrorText();
        return (STATUS_INTERNAL);
    }

    stub.Write(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientES.Write", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    return (::cyprestore::common::pb::STATUS_OK);
}

// Sync
StatusCode RpcClientES::Sync(
        const std::string &extent_id, uint64_t offset, const void *buf,
        uint32_t buflen, const std::string &crc32, std::string &errmsg) {
    cyprestore::extentserver::pb::SyncRequest request;
    cyprestore::extentserver::pb::SyncResponse response;
    request.set_extent_id(extent_id);
    request.set_offset(offset);
    request.set_size(buflen);
    request.set_crc32(crc32);

    cyprestore::extentserver::pb::ExtentService_Stub stub(&channel);
    brpc::Controller cntl;
    if (cntl.request_attachment().append(buf, buflen) == -1) {
        LOG(WARNING) << "RpcClientES.Sync request_attachment().append failed "
                     << cntl.ErrorText();
        return (STATUS_INTERNAL);
    }

    stub.Sync(&cntl, &request, &response, NULL);
    errmsg = response.status().message();
    StatusCode ret = CheckResult(
            "RpcClientES.Sync", &cntl, response.status().code(), errmsg);
    if (ret != STATUS_OK) return (ret);

    return (::cyprestore::common::pb::STATUS_OK);
}

}  // namespace cyprestore
