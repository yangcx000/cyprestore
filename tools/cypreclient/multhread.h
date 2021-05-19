/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */
#ifndef MULTHREAD_H
#define MULTHREAD_H

#include <brpc/channel.h>
#include <brpc/server.h>
#include <bthread/bthread.h>
#include <butil/logging.h>
#include <butil/macros.h>
#include <butil/string_printf.h>
#include <butil/time.h>

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include "access/pb/access.pb.h"
#include "common/pb/types.pb.h"

using namespace cyprestore::common::pb;
using namespace std;

enum JobType { jtWrite, jtRandWrite, jtRead, jtRandRead };

class MulThreadTest {
public:
    MulThreadTest();
    ~MulThreadTest();

    //	bvar::LatencyRecorder m_latency_recorder("MulThreadTest");
    //  bvar::Adder<int> m_error_count("client_error_count");
    butil::static_atomic<int> m_sender_count = BUTIL_STATIC_ATOMIC_INIT(0);

    int Start(int ThreadCount, JobType jobtype, const string &blob_id);
};

#endif
