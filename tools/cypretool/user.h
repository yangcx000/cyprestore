/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_TOOLS_CYPRETOOL_USER_H_
#define CYPRESTORE_TOOLS_CYPRETOOL_USER_H_

#include <brpc/channel.h>

#include <string>

#include "access/pb/access.pb.h"
#include "common/pb/types.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class User {
public:
    User(const Options &options)
            : options_(options), initialized_(false), stub_(nullptr) {}

    int Exec();
    int Create();
    int Delete();
    int Query();
    int List();
    int Update();

private:
    int init();
    void Describe(const common::pb::User &user);
    common::pb::PoolType GetPoolType();

    Options options_;
    bool initialized_;
    brpc::Channel channel_;
    access::pb::AccessService_Stub *stub_;
};

}  // namespace tools
}  // namespace cyprestore

#endif  // CYPRESTORE_TOOLS_CYPRETOOL_USER_H_
