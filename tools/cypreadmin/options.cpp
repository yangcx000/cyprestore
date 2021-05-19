/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "options.h"

#include <iostream>

namespace cyprestore {
namespace tools {

Object GetObject(const std::string &object) {
    if (object == kObjectPool) {
        return Object::kPool;
    } else if (object == kObjectEs) {
        return Object::kExtentServer;
    } else if (object == kObjectRg) {
        return Object::kReplicationGroup;
    } else if (object == kObjectHb) {
        return Object::kHeartbeat;
    }

    return Object::kUnknown;
}

Command GetCommand(const std::string &cmd) {
    if (cmd == kCmdCreate) {
        return Command::kCreate;
    } else if (cmd == kCmdQuery) {
        return Command::kQuery;
    } else if (cmd == kCmdList) {
        return Command::kList;
    } else if (cmd == kCmdRename) {
        return Command::kRename;
    } else if (cmd == kCmdInit) {
        return Command::kInit;
    } else if (cmd == kCmdReport) {
        return Command::kReport;
    } else if (cmd == kCmdDelete) {
        return Command::kDelete;
    }

    return Command::kInvalid;
}

std::string Options::Addr() {
    return em_ip + ":" + std::to_string(em_port);
}

}  // namespace tools
}  // namespace cyprestore
