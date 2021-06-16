/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "log.h"

#include <butil/file_util.h>
#include <butil/files/file_path.h>
#include <butil/logging.h>

#include <iostream>
#include <sstream>
#include <string>

#include "utils/chrono.h"

namespace cyprestore {
namespace common {

int Log::index_ = 0;

int Log::Init() {
    if (!butil::DirectoryExists(butil::FilePath(cfg_.logpath))) {
        // 第二个参数表示是否创建parents
        if (!butil::CreateDirectory(butil::FilePath(cfg_.logpath), true)) {
            std::cerr << "Couldn't create logpath " << cfg_.logpath;
            return -1;
        }
    }

    logging::LoggingSettings settings;
    settings.logging_dest = logging::LOG_TO_FILE;
    settings.log_file = full_path().c_str();

    if (!logging::InitLogging(settings)) return -1;

    logging::SetMinLogLevel(LogLevel());
    return 0;
}

std::string Log::full_path() {
    std::stringstream ss;
    ss << cfg_.logpath << "/" << cfg_.prefix << "."
       << utils::Chrono::DateString() << cfg_.suffix << "." << index_;
    return ss.str();
}

int Log::LogLevel() {
    if (cfg_.level == "INFO") return logging::BLOG_INFO;
    if (cfg_.level == "NOTICE") return logging::BLOG_NOTICE;
    if (cfg_.level == "WARNING") return logging::BLOG_WARNING;
    if (cfg_.level == "ERROR") return logging::BLOG_ERROR;
    if (cfg_.level == "FATAL") return logging::BLOG_FATAL;

    return logging::BLOG_INFO;
}

}  // namespace common
}  // namespace cyprestore
