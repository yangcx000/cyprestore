/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_COMMON_METRICS_H_
#define CYPRESTORE_COMMON_METRICS_H_

#include <butil/time.h>
#include <bvar/bvar.h>

namespace cyprestore {
namespace common {

class MetricsGuard {
public:
    MetricsGuard(bvar::LatencyRecorder *latency_recorder)
            : latency_recorder_(latency_recorder) {
        timer_.start();
    }

    ~MetricsGuard() {}

    void stop() {
        timer_.stop();
        (*latency_recorder_) << timer_.u_elapsed();
    }

private:
    butil::Timer timer_;
    bvar::LatencyRecorder *latency_recorder_;
};

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_METRICS_H_
