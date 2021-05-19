/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#ifndef CYPRESTORE_TOOLS_SET_H_
#define CYPRESTORE_TOOLS_SET_H_

#include <brpc/channel.h>

#include <string>

#include "access/pb/access.pb.h"
#include "common/pb/types.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class Set {
public:
    Set(const Options &options) : options_(options), initialized_(false) {}
    ~Set() = default;

    int Exec();
    int Query();
    // int List();
    void Describe(const common::pb::SetRoute &setroute);

private:
    int init();
    Options options_;
    access::pb::AccessService_Stub *stub_;
    bool initialized_;
    brpc::Channel channel_;
};

}  // namespace tools
}  // namespace cyprestore

#endif  // CYPRESTORE_TOOLS_SET_H_
