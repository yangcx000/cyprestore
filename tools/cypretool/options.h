/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_TOOLS_CYPRETOOL_OPTIONS_H_
#define CYPRESTORE_TOOLS_CYPRETOOL_OPTIONS_H_

#include <string>

namespace cyprestore {
namespace tools {

// Object
const std::string kObjectUser = "user";
const std::string kObjectBlob = "blob";
const std::string kObjectExtent = "extent";
const std::string kObjectSet = "set";
// direct connect extentmanager
const std::string kObjectDUser = "duser";
const std::string kObjectDBlob = "dblob";
const std::string kObjectDRouter = "drouter";
const std::string kObjectScrub = "scrub";

// bench
const std::string kObjectRouterBench = "routerbench";

// Command
const std::string kCmdCreate = "create";
const std::string kCmdDelete = "delete";
const std::string kCmdQuery = "query";
const std::string kCmdList = "list";
const std::string kCmdRename = "rename";
const std::string kCmdResize = "resize";
const std::string kCmdUpdate = "update";
const std::string kCmdSeqRead = "seqread";
const std::string kCmdRandRead = "randread";
const std::string kCmdSeqWrite = "seqwrite";
const std::string kCmdRandWrite = "randwrite";
const std::string kCmdBenchCreate = "benchcreate";
const std::string kCmdBenchQuery = "benchquery";
const std::string kCmdVerify = "verify";

enum Object {
    kUser = 0,
    kBlob,
    kExtent,
    kSet,
    kDUser,
    kDBlob,
    kDRouter,
    kRouterBench,
    kScrub,
    kUnknown = -1,
};

enum Cmd {
    kCreate = 0,
    kDelete,
    kQuery,
    kList,
    kRename,
    kResize,
    kUpdate,
    kSeqRead,
    kRandRead,
    kSeqWrite,
    kRandWrite,
    kBenchCreate,
    kBenchQuery,
    kVerify,
    kInvalid = -1,
};

struct Options {
    std::string AccessAddr() {
        return access_ip + ":" + std::to_string(access_port);
    }

    std::string ExtentManagerAddr() {
        return extentmanager_ip + ":" + std::to_string(extentmanager_port);
    }

    Object obj;
    Cmd cmd;
    /* User */
    std::string user_name;
    std::string user_new_name;
    std::string user_id;
    std::string email;
    std::string new_email;
    std::string comments;
    std::string new_comments;
    std::string pool_id;
    std::string pool_type;
    int32_t set_id;

    /* Blob */
    std::string blob_name;
    std::string blob_new_name;
    std::string blob_id;
    std::string blob_type;
    std::string instance_id;
    std::string blob_desc;
    uint64_t blob_size;
    uint64_t blob_new_size;

    /* Extent */
    std::string extent_id;
    int64_t offset;
    int64_t size;
    int32_t block_size;
    int32_t iodepth;
    int32_t jobs;

    /* router */
    int32_t extent_idx;

    /* Brpc */
    std::string protocal;
    std::string connection_type;
    int32_t timeout_ms;
    int32_t connection_timeout_ms;
    int32_t max_retry;
    int32_t dummy_port;

    /* Access */
    std::string access_ip;
    int32_t access_port;

    /* ExtentManager */
    std::string extentmanager_ip;
    int32_t extentmanager_port;

    /* bench */
    int32_t thread_num;
    int32_t blob_num;
};

Object getObject(const std::string &obj);

Cmd getCmd(const std::string &cmd);

}  // namespace tools
}  // namespace cyprestore

#endif
