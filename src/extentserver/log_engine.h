/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
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
