/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#ifndef RPCCLIENT_ES_H_
#define RPCCLIENT_ES_H_

#include "rpcclient_base.h"

namespace cyprestore {

class RpcClientES : public RpcClient {
public:
    RpcClientES();
    virtual ~RpcClientES();

    StatusCode
    Read(const std::string &extent_id, uint64_t offset, void *buf,
         uint32_t bufsize, uint32_t &retreadlen, std::string &crc32,
         std::string &errmsg);
    StatusCode
    Write(const std::string &extent_id, uint64_t offset, const void *buf,
          uint32_t buflen, const std::string &crc32, std::string &errmsg);
    StatusCode
    Sync(const std::string &extent_id, uint64_t offset, const void *buf,
         uint32_t buflen, const std::string &crc32, std::string &errmsg);
};

}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENT_ES_H_
