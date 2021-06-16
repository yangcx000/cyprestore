/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include <gflags/gflags.h>

#include <iomanip>  // setw
#include <iostream>

#include "access_manager.h"

using namespace cyprestore::access;

DEFINE_string(conf, "../../conf/access.ini", "Ini config file of access");

static void Usage() {
    std::cout << "Usage:" << std::endl
              << std::setw(30) << "access -conf=xxxx" << std::endl;
}

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_conf.empty()) {
        Usage();
        return -1;
    }

    auto &global_access = Access::GlobalInstance();
    if (global_access.Init(FLAGS_conf) != 0) {
        return -1;
    }

    if (global_access.RegistreServices() != 0) {
        return -1;
    }

    global_access.Start();
    return 0;
}
