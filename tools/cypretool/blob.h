/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_TOOLS_CYPRETOOL_BLOB_H_
#define CYPRESTORE_TOOLS_CYPRETOOL_BLOB_H_

#include <brpc/channel.h>

#include <string>

#include "access/pb/access.pb.h"
#include "options.h"

namespace cyprestore {
namespace tools {

class Blob {
public:
    Blob(const Options &options)
            : options_(options), initialized_(false), stub_(nullptr) {}

    int Exec();
    int Create();
    int Delete();
    int Query();
    int List();
    int Rename();
    int Resize();

private:
    int init();
    void describe(const common::pb::Blob &blob);
    common::pb::BlobType getBlobType();
    std::string getBlobType(common::pb::BlobType type);

    Options options_;
    bool initialized_;
    brpc::Channel channel_;
    access::pb::AccessService_Stub *stub_;
};

}  // namespace tools
}  // namespace cyprestore

#endif  // CYPRESTORE_TOOLS_CYPRETOOL_BLOB_H_
