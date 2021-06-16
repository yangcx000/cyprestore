/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include <gflags/gflags.h>

#include <iomanip>  // setw
#include <iostream>

#include "extentmanager/extent_manager.h"

using namespace cyprestore::extentmanager;

DEFINE_string(conf, "", "INI config file of ExtentManager");

static void Usage() {
    std::cout << "Usage:" << std::endl
              << std::setw(30) << "extentmanager -conf=xxxx" << std::endl;
}

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_conf.empty()) {
        Usage();
        return -1;
    }

    auto &global_extentmanager = ExtentManager::GlobalInstance();
    if (global_extentmanager.Init(FLAGS_conf) != 0) return -1;

    if (global_extentmanager.RegisterServices() != 0) return -1;

    global_extentmanager.Start();
    return 0;
}
