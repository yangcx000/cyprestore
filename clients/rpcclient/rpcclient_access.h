/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */
#ifndef RPCCLIENT_ACCESS_H
#define RPCCLIENT_ACCESS_H

#include "access/pb/access.pb.h"
#include "rpcclient_base.h"

namespace cyprestore {

class RpcClientAccess : public RpcClient {
public:
    RpcClientAccess();
    virtual ~RpcClientAccess();

    // user
    StatusCode CreateUser(
            const std::string &user_name, const std::string &email,
            const std::string &comments, int32_t set_id,
            const std::string &pool_id,
            cyprestore::common::pb::PoolType pool_type, std::string &user_id,
            std::string &errmsg);
    StatusCode DeleteUser(const std::string &user_id, std::string &errmsg);
    StatusCode RenameUser(
            const std::string &user_id, const std::string &new_name,
            std::string &errmsg);
    StatusCode
    QueryUser(const std::string &user_id, User &userinfo, std::string &errmsg);
    StatusCode ListUsers(
            int set_id, const int from, const int count, std::list<User> &Users,
            std::string &errmsg);

    // blob
    StatusCode CreateBlob(
            const std::string &user_id, const std::string &blob_name,
            uint64_t blob_size, const BlobType blob_type,
            const std::string &instance_id, const std::string &blob_desc,
            const std::string &pool_id, std::string &blob_id,
            std::string &errmsg);
    StatusCode DeleteBlob(
            const std::string &user_id, const std::string &blob_id,
            std::string &errmsg);
    StatusCode RenameBlob(
            const std::string &user_id, const std::string &blob_id,
            const std::string &new_name, std::string &errmsg);
    StatusCode ResizeBlob(
            const std::string &user_id, const std::string &blob_id,
            uint64_t blob_size, std::string &errmsg);
    StatusCode QueryBlob(
            const std::string &user_id, const std::string &blob_id, Blob &blob,
            std::string &errmsg);
    StatusCode ListBlobs(
            const std::string &user_id, const int from, const int count,
            std::list<Blob> &Blobs, std::string &errmsg);

    StatusCode QuerySet(
            const std::string &user_id,
            cyprestore::common::pb::SetRoute &router, std::string &errmsg);
};

}  // namespace cyprestore

#endif
