/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */
#ifndef BLOB_H
#define BLOB_H

#include "common/pb/types.pb.h"
//#include "access/pb/access.pb.h"
#include <brpc/channel.h>

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

using namespace cyprestore::common::pb;
using namespace std;

namespace cyprestore {

const uint64_t EXTENT_SIZE = 1073741824;  // 1G
const int BLOCK_SIZE = 4096;

//ͬ����д
StatusCode WriteBlob(
        const std::string &ExtentManagerIp, int ExtentManagerPort,
        const std::string &blob_id, uint64_t offset, const unsigned char *buf,
        uint32_t buflen, std::string &errmsg);
StatusCode ReadBlob(
        const std::string &ExtentManagerIp, int ExtentManagerPort,
        const std::string &blob_id, uint64_t offset, unsigned char *buf,
        uint32_t bufsize, uint32_t &retreadlen, std::string &errmsg);

//�첽��д
StatusCode InitBrpcChannel(
        brpc::Channel &channel, const std::string &server_addr, int port);

};  // namespace cyprestore

#endif
