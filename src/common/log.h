/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_COMMON_LOG_H_
#define CYPRESTORE_COMMON_LOG_H_

#include <string>

#include "config.h"

namespace cyprestore {
namespace common {

class Log {
public:
    Log(const LogCfg &cfg) : cfg_(cfg) {}

    int Init();
    int LogLevel();

private:
    Log(const Log &) = delete;
    Log &operator=(const Log &) = delete;
    std::string full_path();

    static int index_;
    LogCfg cfg_;
};

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_LOG_H_
