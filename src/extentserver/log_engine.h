/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_LOG_ENGINE_H
#define CYPRESTORE_EXTENTSERVER_LOG_ENGINE_H

#include <memory>

#include "io_engine.h"

namespace cyprestore {
namespace extentserver {

class LogEngine;
typedef std::shared_ptr<LogEngine> LogEnginePtr;

class LogEngine : public IOEngine {
public:
    LogEngine(StorageEngine *se) : IOEngine(se) {}
    virtual ~LogEngine() {}
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_LOG_ENGINE_H
