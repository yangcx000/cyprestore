/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "options.h"

namespace cyprestore {
namespace tools {

Object getObject(const std::string &obj) {
    if (obj == kObjectUser) {
        return Object::kUser;
    } else if (obj == kObjectBlob) {
        return Object::kBlob;
    } else if (obj == kObjectExtent) {
        return Object::kExtent;
    } else if (obj == kObjectSet) {
        return Object::kSet;
    } else if (obj == kObjectDUser) {
        return Object::kDUser;
    } else if (obj == kObjectDBlob) {
        return Object::kDBlob;
    } else if (obj == kObjectDRouter) {
        return Object::kDRouter;
    } else if (obj == kObjectRouterBench) {
        return Object::kRouterBench;
    } else if (obj == kObjectScrub) {
        return Object::kScrub;
    }

    return Object::kUnknown;
}

Cmd getCmd(const std::string &cmd) {
    if (cmd == kCmdCreate) {
        return Cmd::kCreate;
    } else if (cmd == kCmdDelete) {
        return Cmd::kDelete;
    } else if (cmd == kCmdQuery) {
        return Cmd::kQuery;
    } else if (cmd == kCmdList) {
        return Cmd::kList;
    } else if (cmd == kCmdRename) {
        return Cmd::kRename;
    } else if (cmd == kCmdResize) {
        return Cmd::kResize;
    } else if (cmd == kCmdUpdate) {
        return Cmd::kUpdate;
    } else if (cmd == kCmdSeqRead) {
        return Cmd::kSeqRead;
    } else if (cmd == kCmdRandRead) {
        return Cmd::kRandRead;
    } else if (cmd == kCmdSeqWrite) {
        return Cmd::kSeqWrite;
    } else if (cmd == kCmdRandWrite) {
        return Cmd::kRandWrite;
    } else if (cmd == kCmdBenchCreate) {
        return Cmd::kBenchCreate;
    } else if (cmd == kCmdBenchQuery) {
        return Cmd::kBenchQuery;
    } else if (cmd == kCmdVerify) {
        return Cmd::kVerify;
    }

    return Cmd::kInvalid;
}

}  // namespace tools
}  // namespace cyprestore
