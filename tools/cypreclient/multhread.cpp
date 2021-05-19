/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */
#include "multhread.h"

#include <brpc/channel.h>
#include <bthread/bthread.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include "blobplus.h"
#include "common/pb/types.pb.h"
#include "stringdict.h"

using namespace cyprestore;
using namespace cyprestore::common::pb;
using namespace std;

extern std::string g_ExtentManagerIp;
extern int g_ExtentManagerPort;
extern StringDict g_config;

MulThreadTest::MulThreadTest() {}

MulThreadTest::~MulThreadTest() {}

//多线程写测试
struct IOJob {
    bthread_t th;
    string blob_id;
    MulThreadTest *mulThreadTest;
    JobType jobtype;
};

static void *ThreadWork(void *arg) {

    IOJob *job = (IOJob *)arg;
    MulThreadTest *mulThreadTest = job->mulThreadTest;

    int log_id = 0;

    const int bufsize = 65536;
    char *buf = new char[bufsize];
    memset(buf, 'A', bufsize);
    uint32_t buflen = bufsize;
    uint32_t readlen = 0;
    string errmsg;
    string blob_id = job->blob_id;
    uint64_t BlobSize = 10240000;
    uint64_t offset = 0;
    StatusCode ret;

    const int thread_index = mulThreadTest->m_sender_count.fetch_add(
            1, butil::memory_order_relaxed);
    srand(time(NULL));

    while (!brpc::IsAskedToQuit()) {
        switch (job->jobtype) {
            case jtWrite:
                ret = WriteBlob(
                        g_ExtentManagerIp, g_ExtentManagerPort, blob_id, offset,
                        (unsigned char *)buf, bufsize, errmsg);
                break;
            case jtRandWrite:
                offset = rand() % BlobSize;
                ret = WriteBlob(
                        g_ExtentManagerIp, g_ExtentManagerPort, blob_id, offset,
                        (unsigned char *)buf, bufsize, errmsg);
                break;
            case jtRead:
                ret = ReadBlob(
                        g_ExtentManagerIp, g_ExtentManagerPort, blob_id, offset,
                        (unsigned char *)buf, buflen, readlen, errmsg);
                break;
            case jtRandRead:
                offset = rand() % BlobSize;
                ret = ReadBlob(
                        g_ExtentManagerIp, g_ExtentManagerPort, blob_id, offset,
                        (unsigned char *)buf, buflen, readlen, errmsg);
                break;
        }

        if (ret != STATUS_OK) {
            printf("WriteBlob failed   %s\r\n", errmsg.c_str());
            //	  g_error_count << 1;
            bthread_usleep(50000);
        } else {

            //       g_latency_recorder << cntl.latency_us();
        }
    }

    delete[] buf;

    return (NULL);
}

int MulThreadTest::Start(
        int ThreadCount, JobType jobtype, const string &blob_id) {
    std::vector<IOJob> ThreadArr(ThreadCount);

    for (int i = 0; i < ThreadCount; ++i) {
        ThreadArr[i].blob_id = blob_id;  //先分配好
        ThreadArr[i].mulThreadTest = this;
        ThreadArr[i].jobtype = jobtype;

        if (bthread_start_background(
                    &ThreadArr[i].th, NULL, ThreadWork, (void *)&ThreadArr[i])
            != 0) {
            LOG(ERROR) << "Fail to create bthread";
            return -1;
        }
    }

    while (!brpc::IsAskedToQuit()) {
        sleep(1);
        // LOG(INFO) << "Sending EchoRequest at qps=" <<
        // g_latency_recorder.qps(1) << " latency=" <<
        // g_latency_recorder.latency(1);
    }

    for (int i = 0; i < ThreadCount; ++i) {
        bthread_join(ThreadArr[i].th, NULL);
    }
}
