/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_TOOLS_CYPREADMIN_OPTIONS_H_
#define CYPRESTORE_TOOLS_CYPREADMIN_OPTIONS_H_

#include <cstdint>
#include <string>

namespace cyprestore {
namespace tools {

// Object
const std::string kObjectPool = "pool";
const std::string kObjectEs = "es";
const std::string kObjectRg = "rg";
const std::string kObjectHb = "hb";

// Command
const std::string kCmdCreate = "create";
const std::string kCmdQuery = "query";
const std::string kCmdList = "list";
const std::string kCmdRename = "rename";
const std::string kCmdInit = "init";
const std::string kCmdReport = "report";
const std::string kCmdDelete = "delete";

enum Object {
    kPool = 0,
    kExtentServer,
    kReplicationGroup,
    kHeartbeat,
    kUnknown = -1,
};

enum Command {
    kCreate = 0,
    kQuery,
    kList,
    kRename,
    kInit,
    kReport,
    kDelete,
    kInvalid = -1,
};

struct Options {
    std::string Addr();

    std::string object;
    std::string cmd;

    /* Brpc */
    std::string protocal;
    std::string connection_type;
    int32_t timeout_ms;
    int32_t connection_timeout_ms;
    int32_t max_retry;

    /* Pool */
    std::string pool_name;
    std::string pool_new_name;
    std::string pool_id;
    std::string pool_type;
    std::string failure_domain;
    int32_t replica_count;
    int32_t rg_count;
    int32_t es_count;

    /* ExtentServer */
    int32_t es_id;
    std::string es_host;
    std::string es_rack;

    /* Replication Group */
    std::string rg_id;

    /* ExtentManager */
    std::string em_ip;
    int32_t em_port;
};

Object GetObject(const std::string &object);

Command GetCommand(const std::string &cmd);

}  // namespace tools
}  // namespace cyprestore

#endif
