/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTSERVER_HEARTBEAT_REPORTER_H_
#define CYPRESTORE_EXTENTSERVER_HEARTBEAT_REPORTER_H_

#include <butil/macros.h>

#include <memory>

#include "common/status.h"

namespace cyprestore {
namespace extentserver {

class ExtentServer;
class HeartbeatReporter;

typedef std::shared_ptr<HeartbeatReporter> HeartbeatReporterPtr;

using common::Status;

class HeartbeatReporter {
public:
    HeartbeatReporter(int heartbeat_interval_sec, ExtentServer *es)
            : heartbeat_interval_sec_(heartbeat_interval_sec), es_(es),
              once_(false) {}
    ~HeartbeatReporter() = default;

    static void HeartbeatEntry(void *arg);
    Status Start();

private:
    DISALLOW_COPY_AND_ASSIGN(HeartbeatReporter);
    void DoHeartbeatReport();

    // TODO(yangchunxin3): 类型重构为long
    int heartbeat_interval_sec_;
    ExtentServer *es_;
    bool once_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_HEARTBEAT_REPORTER_H_
