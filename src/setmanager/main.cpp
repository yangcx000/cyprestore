/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

#include <iomanip>  // setw

#include "setmanager.h"

using namespace cyprestore::setmanager;

DEFINE_string(conf, "", "INI Config file of SetManager");

static void Usage() {
    std::cout << "Usage:" << std::endl
              << std::setw(30) << "setmanager -conf=xxxx" << std::endl;
}

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_conf.empty()) {
        Usage();
        return -1;
    }

    auto &global_setmanager = SetManager::GlobalInstance();
    if (global_setmanager.Init(FLAGS_conf) != 0) return -1;

    if (global_setmanager.RegisterServices() != 0) return -1;

    global_setmanager.Start();
    return 0;
}
