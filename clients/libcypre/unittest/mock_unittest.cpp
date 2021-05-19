
#include <brpc/channel.h>
#include <bthread/bthread.h>
#include <sys/time.h>

#include "common/connection_pool.h"
#include "common/extent_id_generator.h"
#include "common/extent_router.h"
#include "gtest/gtest.h"
#include "libcypre.h"
#include "libcypre_common.h"
#include "mock/mock_service.h"
#include "stream/rbd_stream_handle_impl.h"
#include "utils/crc32.h"

using namespace cyprestore;
using namespace cyprestore::clients;
using namespace cyprestore::clients::mock;

static inline uint64_t time_diff(struct timespec &v1, struct timespec &v2) {
    return (v2.tv_sec - v1.tv_sec) * 1000000 + (v2.tv_nsec - v1.tv_nsec) / 1000;
}
class MockTest : public ::testing::Test {
protected:
    void SetUp() override {
        rbd_ = CypreRBD::New();
        mock_ = NULL;
    }

    void TearDown() override {
        if (mock_ != NULL) {
            mock_->Stop();
        }
        delete rbd_;
        rbd_ = NULL;
        delete mock_;
        mock_ = NULL;
    }
    int prepareForLatencyTest(
            uint64_t dev_size, int mport, const std::string &blob_name,
            bool nullblk, RBDStreamHandlePtr &handle);
    MockInstance *mock_;
    CypreRBD *rbd_;
};

int MockTest::prepareForLatencyTest(
        uint64_t dev_size, int mport, const std::string &blob_name,
        bool nullblk, RBDStreamHandlePtr &handle) {
    MockExtentManager *mem = NULL;
    MockLogicManager *mlm = new MockLogicManager();
    if (nullblk) {
        mem = new MockEmptyExtentManager();
    } else {
        mem = new MockMemExtentManager();
    }
    mock_ = new MockInstance(mlm, mem);
    int rv = mock_->StartMaster("127.0.0.1", mport);
    if (rv != 0) {
        LOG(ERROR) << "start master failed";
        return rv;
    }
    std::vector<MockInstance::Address> lles = { { "127.0.0.1", mport + 1 },
                                                { "127.0.0.1", mport + 2 } };
    rv = mock_->StartServer(lles);
    if (rv != 0) {
        LOG(ERROR) << "start servers failed";
        return rv;
    }
    MockMasterClient client;
    brpc::Controller cntl;
    char addr[100];
    snprintf(addr, sizeof(addr) - 1, "127.0.0.1:%d", mport);
    rv = client.Init(addr);
    if (rv != 0) {
        LOG(ERROR) << "connect to manager failed";
        return rv;
    }
    // create user & blob
    std::string uid1;
    rv = client.CreateUser("sk1", "sk1@jd.com", uid1);
    if (rv != 0 || uid1.empty()) {
        LOG(ERROR) << "create user failed";
        return rv;
    }
    // create blob
    std::string bid1;
    rv = client.CreateBlob(
            uid1, blob_name, blob_name + ".test",
            common::pb::BlobType::BLOB_TYPE_EXCELLENT, dev_size, bid1);
    if (rv != 0 || bid1.empty()) {
        LOG(ERROR) << "createblob failed";
        return rv;
    }
    // init cypre client
    CypreRBDOptions opt("127.0.0.1", mport);
    opt.brpc_sender_ring_power = 21;
    opt.brpc_sender_thread = 4;
    rv = rbd_->Init(opt);
    if (rv != 0) {
        LOG(ERROR) << "init cypre rbd failed";
        return rv;
    }
    rv = rbd_->Open(bid1, handle);
    if (rv != 0) {
        LOG(ERROR) << "open bdev failed";
    }
    return rv;
}

TEST_F(MockTest, TestMockMaster) {
    mock_ = new MockInstance();
    int rv = mock_->StartMaster("127.0.0.1", 12345);
    ASSERT_TRUE(rv == 0);

    std::vector<MockInstance::Address> lles = { { "127.0.0.1", 12300 } };
    rv = mock_->StartServer(lles);
    ASSERT_TRUE(rv == 0);
    MockMasterClient client;
    brpc::Controller cntl;
    if (client.Init("127.0.0.1:12345") != 0) {
        LOG(ERROR) << "connect to manager failed";
        ASSERT_TRUE(false);
    }
    // create user
    std::string uid1, uid2;
    rv = client.CreateUser("sk1", "sk1@jd.com", uid1);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(!uid1.empty());
    rv = client.CreateUser("sk2", "sk2@jd.com", uid2);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(!uid2.empty());
    rv = client.CreateUser("sk2", "sk2@jd.com", uid2);
    ASSERT_TRUE(rv != 0);
    // query user
    common::pb::User user;
    rv = client.QueryUser(uid1, user);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(user.id() == uid1);
    rv = client.QueryUser("none", user);
    ASSERT_TRUE(rv != 0);
    // list user
    std::vector<common::pb::User> lluser;
    rv = client.ListUser(lluser);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(lluser.size() == 2);
    ASSERT_TRUE(lluser[0].id() == uid1);  // smaller uid
    // create blob
    std::string bid1, bid2;
    rv = client.CreateBlob(
            uid1, "blob1", "tblob1", common::pb::BlobType::BLOB_TYPE_EXCELLENT,
            100 * 1024 * 1024, bid1);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(!bid1.empty());
    rv = client.CreateBlob(
            uid1, "blob2", "tblob2", common::pb::BlobType::BLOB_TYPE_EXCELLENT,
            200 * 1024 * 1024, bid2);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(!bid2.empty());
    rv = client.CreateBlob(
            uid2, "blob3", "tblob3", common::pb::BlobType::BLOB_TYPE_EXCELLENT,
            200 * 1024 * 1024, bid2);
    ASSERT_TRUE(rv == 0);
    // query blob
    common::pb::Blob blob;
    rv = client.QueryBlob(bid1, blob);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(blob.id() == bid1);
    rv = client.QueryBlob("none", blob);
    ASSERT_TRUE(rv != 0);
    // query blob
    std::vector<common::pb::Blob> llblob;
    rv = client.ListBlob(uid1, llblob);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(llblob.size() == 2);
    ASSERT_TRUE(llblob[0].id() == bid1);  // smaller bid
    // query router
    common::pb::ExtentRouter router;
    std::string extent_id =
            common::ExtentIDGenerator::GenerateExtentID(bid1, 1);
    rv = client.QueryRouter(extent_id, router);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(router.primary().public_ip() == "127.0.0.1");
    ASSERT_TRUE(router.primary().public_port() == 12300);
}

TEST_F(MockTest, TestMockExtentServer) {
    const int ESIZE = 1024 * 1024 * 2;
    GlobalConfig::Instance()->SetExtentSize(ESIZE);
    mock_ = new MockInstance();
    int rv = mock_->StartMaster("127.0.0.1", 12345);
    ASSERT_TRUE(rv == 0);

    std::vector<MockInstance::Address> lles = { { "127.0.0.1", 12300 } };
    rv = mock_->StartServer(lles);
    ASSERT_TRUE(rv == 0);
    MockMasterClient client;
    brpc::Controller cntl;
    if (client.Init("127.0.0.1:12345") != 0) {
        LOG(ERROR) << "connect to manager failed";
        ASSERT_TRUE(false);
    }
    // create user & blob
    std::string uid1;
    rv = client.CreateUser("sk1", "sk1@jd.com", uid1);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(!uid1.empty());
    // create blob
    std::string bid1;
    rv = client.CreateBlob(
            uid1, "blob1", "tblob1", common::pb::BlobType::BLOB_TYPE_EXCELLENT,
            100 * 1024 * 1024, bid1);
    ASSERT_TRUE(rv == 0);
    ASSERT_TRUE(!bid1.empty());
    // write data & read
    // we can read/write partial data in mem block device
    const std::string data =
            "12345678901234567890abcdefghijklmnxxxx!!!!#@#@&*(&^((^^";
    rv = client.Write(bid1, ESIZE, data.size(), data);
    ASSERT_TRUE(rv == 0);
    char buf[10240] = "";
    rv = client.Read(bid1, ESIZE, data.size(), buf);
    ASSERT_TRUE(rv == 0);
    rv = memcmp(data.data(), buf, data.size());
    ASSERT_TRUE(rv == 0);
}

struct brpc_ctx_t {
    brpc_ctx_t(int n) : N(n), fini(0), inflight(0) {}
    int N;
    std::atomic<int> fini;
    std::atomic<int> inflight;
};

static void brpc_io_cb(int status, void *arg) {
    struct brpc_ctx_t *ctx = (struct brpc_ctx_t *)arg;
    ctx->fini++;
    ctx->inflight--;
}

struct sender_arg {
    sender_arg(
            int c, const std::string &bid, int bs, uint64_t ds, const char *w,
            struct brpc_ctx_t *t)
            : count(c), block_size(bs), device_size(ds), wbuf(w), ctx(t),
              blob_id(bid) {}
    int count;
    int block_size;
    int64_t device_size;
    const char *wbuf;
    struct brpc_ctx_t *ctx;
    std::string blob_id;
    CypreRBD *rbd;
};

static void *sender_loop(void *arg) {
    struct sender_arg *sender = (struct sender_arg *)arg;
    RBDStreamHandlePtr handle;
    int rv = sender->rbd->Open(sender->blob_id, handle);
    if (rv != 0) {
        std::cout << "sender_loop:open blob failed:" << sender->blob_id
                  << std::endl;
        exit(-1);
    }
    handle->SetExtentIoProto(ExtentIoProtocol::kNull);
    int offset = rand() % (1024 * 128) * sender->block_size;
    for (int i = 0; i < sender->count; i++) {
        offset += sender->block_size;
        if (offset >= sender->device_size) {
            offset = 0;
        }
        sender->ctx->inflight++;
        rv = handle->AsyncWrite(
                sender->wbuf, sender->block_size, offset, brpc_io_cb,
                sender->ctx);
        if (rv != 0) {
            std::cout << "sender_loop:writefailed:" << sender->blob_id
                      << std::endl;
            exit(-1);
        }
    }
    sender->rbd->Close(handle);
    return NULL;
}

TEST_F(MockTest, TestBrpcIoLatencyAsync) {
    const int BS = 4096;
    const uint64_t DEVICE_SIZE = 256 * 1024 * 1024;
    RBDStreamHandlePtr handle;
    int rv = prepareForLatencyTest(DEVICE_SIZE, 11335, "blob1", true, handle);
    ASSERT_TRUE(rv == 0);
    // write data & read
    char *wbuf = new char[BS];
    for (int i = 0; i < BS; i++) {
        wbuf[i] = rand() % 26 + 'a';
    }
    const int N = 20 * 10000;
    brpc_ctx_t bctx(N);
    struct timespec tvs, tve;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    for (uint32_t i = 0, offset = 0; i < N; i++) {
        while (bctx.inflight >= 128) {
            usleep(1);
        }
        bctx.inflight++;
        rv = handle->AsyncWrite(wbuf, BS, offset, brpc_io_cb, &bctx);
        ASSERT_TRUE(rv == 0);
        offset += BS;
        if (offset >= DEVICE_SIZE) {
            offset = 0;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    uint64_t used = time_diff(tvs, tve);
    LOG(INFO) << "total:" << N
              << ", submit write brpc time used:" << (used / 1000)
              << "ms, per:" << used * 1.0f / N << "us";
    while (bctx.fini < N) usleep(1000);
    clock_gettime(CLOCK_MONOTONIC, &tve);
    used = time_diff(tvs, tve);
    LOG(INFO) << "total:" << N << ", write brpc time used:" << (used / 1000)
              << "ms, per:" << used * 1.0f / N << "us";
    LOG(INFO) << "     iops:" << N * 1000 / (used / 1000);
    rbd_->Close(handle);
}

TEST_F(MockTest, TestCrc32Check) {
    const int BS = 4096;
    const uint64_t DEVICE_SIZE = 256*1024*1024;
    RBDStreamHandlePtr handle;
    int rv = prepareForLatencyTest(DEVICE_SIZE, 11335, "blob1", false, handle);
    ASSERT_TRUE(rv == 0);

    // write test
    char *wbuf = new char[BS];
    for(int i = 0; i < BS; i++) {
        wbuf[i] = rand()%26 + 'a';
    }
    uint32_t wbuf_crc32 = utils::Crc32::Checksum(wbuf, BS);
    LOG(INFO)<<"wbuf crc32: "<< wbuf_crc32;
    const int N = 20*10000;
    brpc_ctx_t wbctx(N);
    struct timespec tvs, tve;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    for(uint32_t i = 0, offset = 0; i < N; i++) {
        while(wbctx.inflight >= 128) {
            usleep(1);
        }
        wbctx.inflight++;
        rv = handle->AsyncWrite(wbuf, BS, offset, brpc_io_cb, &wbctx);
        ASSERT_TRUE(rv == 0);
        offset += BS;
        if(offset >= DEVICE_SIZE) {
            offset = 0;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    uint64_t used = time_diff(tvs, tve);
    LOG(INFO)<<"total:"<<N<<", submit write brpc time used:"<<(used/1000)<<"ms, per:"<<used*1.0f/N<<"us";
    while(wbctx.fini < N) usleep(1000);
    clock_gettime(CLOCK_MONOTONIC, &tve);
    used = time_diff(tvs, tve);
    LOG(INFO)<<"total:"<<N<<", write brpc time used:"<<(used/1000)<<"ms, per:"<<used*1.0f/N<<"us";
    LOG(INFO)<<"     iops:"<<N*1000/(used/1000);
    LOG(INFO)<<"==================================================================";

    // read test
    char *rbuf = new char[BS];
    brpc_ctx_t rbctx(N);
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    for(uint32_t i = 0, offset = 0; i < N; i++) {
        while(rbctx.inflight >= 128) {
            usleep(1);
        }
        rbctx.inflight++;
        rv = handle->AsyncRead(rbuf, BS, offset, brpc_io_cb, &rbctx);
        ASSERT_TRUE(rv == 0);
        offset += BS;
        if(offset >= DEVICE_SIZE) {
            offset = 0;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    used = time_diff(tvs, tve);
    LOG(INFO)<<"total:"<<N<<", submit read brpc time used:"<<(used/1000)<<"ms, per:"<<used*1.0f/N<<"us";
    while(rbctx.fini < N) usleep(1000);
    clock_gettime(CLOCK_MONOTONIC, &tve);
    used = time_diff(tvs, tve);
    LOG(INFO)<<"total:"<<N<<", read brpc time used:"<<(used/1000)<<"ms, per:"<<used*1.0f/N<<"us";
    LOG(INFO)<<"     iops:"<<N*1000/(used/1000);
    LOG(INFO)<<"==================================================================";
}

TEST_F(MockTest, TestBrpcNullIoLatencyAsync) {
    const int BS = 4096;
    const uint64_t DEVICE_SIZE = 256 * 1024 * 1024;
    RBDStreamHandlePtr handle;
    int rv = prepareForLatencyTest(DEVICE_SIZE, 11335, "blob1", true, handle);
    ASSERT_TRUE(rv == 0);
    handle->SetExtentIoProto(ExtentIoProtocol::kNull);
    // write data & read
    char *wbuf = new char[BS];
    for (int i = 0; i < BS; i++) {
        wbuf[i] = rand() % 26 + 'a';
    }
    const int N = 50 * 10000;
    brpc_ctx_t bctx(N);
    struct timespec tvs, tve;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    for (uint32_t i = 0, offset = 0; i < N; i++) {
        while (bctx.inflight >= 1024) {
            usleep(1);
        }
        bctx.inflight++;
        rv = handle->AsyncWrite(wbuf, BS, offset, brpc_io_cb, &bctx);
        ASSERT_TRUE(rv == 0);
        offset += BS;
        if (offset >= DEVICE_SIZE) {
            offset = 0;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    uint64_t used = time_diff(tvs, tve);
    LOG(INFO) << "total:" << N
              << ", submit write brpc time used:" << (used / 1000)
              << "ms, per:" << used * 1.0f / N << "us";
    while (bctx.fini < N) usleep(1000);
    clock_gettime(CLOCK_MONOTONIC, &tve);
    used = time_diff(tvs, tve);
    LOG(INFO) << "total:" << N << ", write brpc time used:" << (used / 1000)
              << "ms, per:" << used * 1.0f / N << "us";
    LOG(INFO) << "     iops:" << N * 1000 / (used / 1000);
}

TEST_F(MockTest, TestBrpcNullIoLatencyAsyncMultithread) {
    const int BS = 4096;
    const uint64_t DEVICE_SIZE = 256 * 1024 * 1024;
    RBDStreamHandlePtr handle;
    int rv = prepareForLatencyTest(DEVICE_SIZE, 11335, "blob1", true, handle);
    ASSERT_TRUE(rv == 0);
    // write data & read
    char *wbuf = new char[BS];
    for (int i = 0; i < BS; i++) {
        wbuf[i] = rand() % 26 + 'a';
    }
    const int M = 4;
    const int N = 100 * 10000;
    brpc_ctx_t bctx(N);
    struct timespec tvs, tve;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    pthread_t tids[M];
    for (int i = 0; i < M; i++) {
        struct sender_arg *sender = new sender_arg(
                N / M, handle->GetDeviceId(), BS, DEVICE_SIZE, wbuf, &bctx);
        sender->rbd = rbd_;
        rv = pthread_create(&tids[i], NULL, sender_loop, sender);
        ASSERT_TRUE(rv == 0);
    }
    for (int i = 0; i < M; i++) {
        pthread_join(tids[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    uint64_t used = time_diff(tvs, tve);
    LOG(INFO) << "total:" << N
              << ", submit write brpc time used:" << (used / 1000)
              << "ms, per:" << used * 1.0f * M / N << "us";
    while (bctx.fini < N) usleep(1000);
    clock_gettime(CLOCK_MONOTONIC, &tve);
    used = time_diff(tvs, tve);
    LOG(INFO) << "total:" << N << ", write brpc time used:" << (used / 1000)
              << "ms, per:" << used * 1.0f / N << "us";
    LOG(INFO) << "     iops:" << N * 1000 / (used / 1000);
}

TEST_F(MockTest, TestBrpcIoLatencySync) {
    const int BS = 4096;
    const uint64_t DEVICE_SIZE = 128 * 1024 * 1024;
    RBDStreamHandlePtr handle;
    int rv = prepareForLatencyTest(DEVICE_SIZE, 9335, "sblob2", true, handle);
    ASSERT_TRUE(rv == 0);
    // write data & read
    char *wbuf = new char[BS];
    for (int i = 0; i < BS; i++) {
        wbuf[i] = rand() % 26 + 'a';
    }
    struct timespec tvs, tve;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    const int N = 5 * 10000;
    for (uint32_t i = 0, offset = 0; i < N; i++) {
        rv = handle->Write(wbuf, BS, offset);
        ASSERT_TRUE(rv == 0);
        offset += BS;
        if (offset >= DEVICE_SIZE) {
            offset = 0;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    uint64_t used = time_diff(tvs, tve);
    LOG(INFO) << "total:" << N << ", write brpc time used:" << (used / 1000)
              << "ms, per:" << used * 1.0f / N << "us";
    rbd_->Close(handle);
}

TEST_F(MockTest, TestNullIoLatency) {
    const int BS = 4096;
    const uint64_t DEVICE_SIZE = 128 * 1024 * 1024;
    RBDStreamHandlePtr handle;
    int rv = prepareForLatencyTest(DEVICE_SIZE, 22345, "blob3", true, handle);
    ASSERT_TRUE(rv == 0);
    handle->SetExtentIoProto(ExtentIoProtocol::kNull);
    // write data & read
    char *wbuf = new char[BS];
    for (int i = 0; i < BS; i++) {
        wbuf[i] = rand() % 26 + 'a';
    }
    struct timespec tvs, tve;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    const int N = 100 * 10000;
    for (uint32_t i = 0, offset = 0; i < N; i++) {
        rv = handle->Write(wbuf, BS, offset);
        ASSERT_TRUE(rv == 0);
        offset += BS;
        if (offset >= DEVICE_SIZE) {
            offset = 0;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    uint64_t used = time_diff(tvs, tve);
    LOG(INFO) << "total:" << N << ", write null time used:" << (used / 1000)
              << "ms, per:" << used * 1.0f / N << "us";
}

static void *send_write_request(void *arg) {
    usleep(1);
    return NULL;
}
TEST_F(MockTest, Testbthread) {
    const int N = 1000 * 1000;
    bthread_t *bths = new bthread_t[N];
    struct timespec tvs, tve;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    for (int i = 0; i < N; i++) {
        int rv = bthread_start_background(
                &bths[i], NULL, send_write_request, NULL);
        ASSERT_EQ(rv, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    {
        uint64_t used = time_diff(tvs, tve);
        std::cout << "total:" << N << ", bthread start per:" << used * 1.0f / N
                  << "us" << std::endl;
    }
    for (int i = 0; i < N; i++) {
        bthread_join(bths[i], NULL);
    }
    delete[] bths;
}
