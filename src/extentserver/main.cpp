/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include <gflags/gflags.h>

#include <iomanip>  // setw
#include <iostream>

#include "extentserver/extentserver.h"

using namespace cyprestore::extentserver;

DEFINE_string(conf, "", "INI config file of ExtentServer");

static void Usage() {
    std::cout << "Usage:" << std::endl
              << std::setw(30) << "extentserver -conf=xxxx" << std::endl;
}

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_conf.empty()) {
        Usage();
        return -1;
    }

    auto &global_extentserver = ExtentServer::GlobalInstance();
    if (global_extentserver.Init(FLAGS_conf) != 0) return -1;

    if (global_extentserver.RegisterServices() != 0) return -1;

    global_extentserver.Start();
    return 0;
}
