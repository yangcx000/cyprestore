
#include <iostream>

#include "concurrency/io_concurrency.h"
#include "gtest/gtest.h"

using namespace cyprestore;
using namespace cyprestore::clients;

static void user_cb(int status, void *ctx) {
    (void)(ctx);
    (void)(status);
}

static uint64_t tdiff(struct timespec &v1, struct timespec &v2) {
    return (v2.tv_sec - v1.tv_sec) * 1000000 + (v2.tv_nsec - v1.tv_nsec) / 1000;
}

class IoConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // clear first
        IoConcurrency::Iterator *ioitr = ioc_.NewIter();
        while (ioitr->hasNext()) {
            const UserIoRequest *ioreq = ioitr->Next();
            delete ioreq;
        }
        delete ioitr;
    }

    void TearDown() override {}
    UserWriteRequest *NewWriteReq(uint64_t off, uint32_t len);
    UserReadRequest *NewReadReq(uint64_t off, uint32_t len);
    IoConcurrency ioc_;
};

UserWriteRequest *IoConcurrencyTest::NewWriteReq(uint64_t off, uint32_t len) {
    UserWriteRequest *req = new UserWriteRequest();
    req->buf = NULL;
    req->logic_offset = off;
    req->logic_len = len;
    req->user_cb = user_cb;
    req->user_ctx = NULL;
    return req;
}

UserReadRequest *IoConcurrencyTest::NewReadReq(uint64_t off, uint32_t len) {
    UserReadRequest *req = new UserReadRequest();
    req->buf = NULL;
    req->logic_offset = off;
    req->logic_len = len;
    req->user_cb = user_cb;
    req->user_ctx = NULL;
    return req;
}

TEST_F(IoConcurrencyTest, TestNoOverlap) {
    UserWriteRequest *req1 = NewWriteReq(0, 4096);
    bool b1 = ioc_.SendWriteRequest(req1, true);
    ASSERT_TRUE(b1);
    UserWriteRequest *req2 = NewWriteReq(4096, 4096);
    bool b2 = ioc_.SendWriteRequest(req2, true);
    ASSERT_TRUE(b2);
    UserWriteRequest *req3 = NewWriteReq(8192, 8912);
    bool b3 = ioc_.SendWriteRequest(req3, true);
    ASSERT_TRUE(b3);
    UserReadRequest *req4 = NewReadReq(20480, 8912);
    bool b4 = ioc_.SendReadRequest(req4, true);
    ASSERT_TRUE(b4);
}

TEST_F(IoConcurrencyTest, TestOverlapLeft1) {
    UserWriteRequest *req1 = NewWriteReq(0, 4096);
    bool b1 = ioc_.SendWriteRequest(req1, true);
    ASSERT_TRUE(b1);
    UserWriteRequest *req2 = NewWriteReq(4096, 8192);
    bool b2 = ioc_.SendWriteRequest(req2, true);
    ASSERT_TRUE(b2);
    UserWriteRequest *req31 = NewWriteReq(8192, 8912);
    bool b31 = ioc_.SendWriteRequest(req31, true);
    ASSERT_FALSE(b31);
    UserReadRequest *req32 = NewReadReq(8192, 8912);
    bool b32 = ioc_.SendReadRequest(req32, true);
    ASSERT_FALSE(b32);
    // check no overlap again
    UserWriteRequest *req4 = NewWriteReq(20480, 8912);
    bool b4 = ioc_.SendWriteRequest(req4, true);
    ASSERT_TRUE(b4);

    uint64_t ps, ds;
    ioc_.GetStats(ds, ps);
    ASSERT_EQ(ps, 2);
    ASSERT_EQ(ds, 3);
}

TEST_F(IoConcurrencyTest, TestOverlapLeft2) {
    UserWriteRequest *req1 = NewWriteReq(0, 4096);
    bool b1 = ioc_.SendWriteRequest(req1, true);
    ASSERT_TRUE(b1);
    UserWriteRequest *req2 = NewWriteReq(4096, 8192);
    bool b2 = ioc_.SendWriteRequest(req2, true);
    ASSERT_TRUE(b2);
    UserWriteRequest *req3 = NewWriteReq(20480, 8912);
    bool b3 = ioc_.SendWriteRequest(req3, true);
    ASSERT_TRUE(b3);
    UserWriteRequest *req41 = NewWriteReq(8192, 4096);
    bool b41 = ioc_.SendWriteRequest(req41, true);
    ASSERT_FALSE(b41);
    UserReadRequest *req42 = NewReadReq(8192, 4096);
    bool b42 = ioc_.SendReadRequest(req42, true);
    ASSERT_FALSE(b42);
    // check no overlap again
    UserWriteRequest *req5 = NewWriteReq(20480 - 4096, 4096);
    bool b5 = ioc_.SendWriteRequest(req5, true);
    ASSERT_TRUE(b5);
}

TEST_F(IoConcurrencyTest, TestOverlapRight1) {
    UserWriteRequest *req1 = NewWriteReq(0, 4096);
    bool b1 = ioc_.SendWriteRequest(req1, true);
    ASSERT_TRUE(b1);
    UserWriteRequest *req2 = NewWriteReq(4096, 8192);
    bool b2 = ioc_.SendWriteRequest(req2, true);
    ASSERT_TRUE(b2);
    UserWriteRequest *req31 = NewWriteReq(4096, 4096);
    bool b31 = ioc_.SendWriteRequest(req31, true);
    ASSERT_FALSE(b31);
    UserReadRequest *req32 = NewReadReq(8192, 8912);
    bool b32 = ioc_.SendReadRequest(req32, true);
    ASSERT_FALSE(b32);
    // check no overlap again
    UserReadRequest *req5 = NewReadReq(20480 - 4096, 4096);
    bool b5 = ioc_.SendReadRequest(req5, true);
    ASSERT_TRUE(b5);
}

TEST_F(IoConcurrencyTest, TestOverlapRight2) {
    UserWriteRequest *req1 = NewWriteReq(0, 4096);
    bool b1 = ioc_.SendWriteRequest(req1, true);
    ASSERT_TRUE(b1);
    UserWriteRequest *req2 = NewWriteReq(8912, 8192);
    bool b2 = ioc_.SendWriteRequest(req2, true);
    ASSERT_TRUE(b2);
    UserWriteRequest *req31 = NewWriteReq(4096, 8192);
    bool b31 = ioc_.SendWriteRequest(req31, true);
    ASSERT_FALSE(b31);
    UserReadRequest *req32 = NewReadReq(4096, 8192);
    bool b32 = ioc_.SendReadRequest(req32, true);
    ASSERT_FALSE(b32);
}

TEST_F(IoConcurrencyTest, TestOverlapWithDone) {
    UserWriteRequest *req1 = NewWriteReq(0, 4096);
    bool b1 = ioc_.SendWriteRequest(req1, true);
    ASSERT_TRUE(b1);
    UserWriteRequest *req2 = NewWriteReq(4096, 8192);
    bool b2 = ioc_.SendWriteRequest(req2, true);
    ASSERT_TRUE(b2);
    UserWriteRequest *req3 = NewWriteReq(20480, 8912);
    bool b3 = ioc_.SendWriteRequest(req3, true);
    ASSERT_TRUE(b3);
    UserReadRequest *req41 = NewReadReq(8192, 4096);
    bool b41 = ioc_.SendReadRequest(req41, true);
    ASSERT_FALSE(b41);
    UserWriteRequest *req42 = NewWriteReq(8192, 4096);
    bool b42 = ioc_.SendWriteRequest(req42, true);
    ASSERT_FALSE(b42);

    UserReadRequest *req51 = NewReadReq(20480 + 4096, 4096);
    bool b51 = ioc_.SendReadRequest(req51, true);
    ASSERT_FALSE(b51);

    UserReadRequest *req61 = NewReadReq(40960, 4096);
    bool b61 = ioc_.SendReadRequest(req61, true);
    ASSERT_TRUE(b61);
    UserReadRequest *req62 = NewReadReq(409600, 4096);
    bool b62 = ioc_.SendReadRequest(req62, true);
    ASSERT_TRUE(b62);

    uint64_t ps, ds;
    ioc_.GetStats(ds, ps);
    ASSERT_EQ(ps, 3);
    ASSERT_EQ(ds, 5);
    // done req2 & req 41
    UserIoRequest *nextio = NULL;
    bool rv1 = ioc_.OnWriteDone(req2->logic_offset, req2->logic_len, &nextio);
    ASSERT_TRUE(rv1);
    ASSERT_EQ(nextio->GetIoType(), kRead);
    ASSERT_EQ(nextio, req41);
    bool rv2 = ioc_.OnReadDone(req41->logic_offset, req41->logic_len, &nextio);
    ASSERT_TRUE(rv2);
    ASSERT_EQ(nextio->GetIoType(), kWrite);
    ASSERT_EQ(nextio, req42);

    ioc_.GetStats(ds, ps);
    ASSERT_EQ(ps, 1);
    ASSERT_EQ(ds, 3);

    // pending new req overlap with req42
    UserReadRequest *req43 = NewReadReq(8192, 4096);
    bool b43 = ioc_.SendReadRequest(req43, true);
    ASSERT_FALSE(b43);

    // other req
    UserReadRequest *req63 = NewReadReq(409600 - 4096, 8192);
    bool b63 = ioc_.SendReadRequest(req63, true);
    ASSERT_FALSE(b63);

    UserReadRequest *req71 = NewReadReq(4096000, 4096);
    bool b71 = ioc_.SendReadRequest(req71, true);
    ASSERT_TRUE(b71);

    ioc_.GetStats(ds, ps);
    ASSERT_EQ(ps, 3);
    ASSERT_EQ(ds, 4);
}

TEST_F(IoConcurrencyTest, TestOverlapWithDone2) {
    UserWriteRequest *wreq[1024];
    UserReadRequest *rreq[1024];
    const size_t maxoff = 1024 * 1024 * 2000;
    for (size_t i = 0; i < 1024; i++) {
        size_t offset = maxoff - i * 1024000;
        wreq[i] = NewWriteReq(offset, 10240);
        rreq[i] = NewReadReq(offset + 102400, 20480);
        bool b1 = ioc_.SendWriteRequest(wreq[i], true);
        ASSERT_TRUE(b1);
        bool b2 = ioc_.SendReadRequest(rreq[i], true);
        ASSERT_TRUE(b2);
    }
    // pending some
    UserWriteRequest *req11 =
            NewWriteReq(maxoff - 1024000 * 300 + 102400 - 4096, 40960);
    bool b11 = ioc_.SendWriteRequest(req11, true);
    ASSERT_FALSE(b11);
    UserWriteRequest *req12 = NewWriteReq(maxoff - 4096, 40960);
    bool b12 = ioc_.SendWriteRequest(req12, true);
    ASSERT_FALSE(b12);

    // done read req 300
    UserIoRequest *nextio = NULL;
    bool rv1 = ioc_.OnReadDone(
            rreq[300]->logic_offset, rreq[300]->logic_len, &nextio);
    ASSERT_TRUE(rv1);
    ASSERT_EQ(nextio->GetIoType(), kWrite);
    ASSERT_EQ(nextio, req11);

    uint64_t ps, ds;
    ioc_.GetStats(ds, ps);
    ASSERT_EQ(ps, 1);
    ASSERT_EQ(ds, 1024 * 2 - 1);
}

TEST_F(IoConcurrencyTest, TestPerf100w) {
    const int N = 10000 * 100;
    UserWriteRequest **wreq = new UserWriteRequest *[N];
    for (size_t i = 0; i < N; i++) {
        size_t offset = i * 4096;
        wreq[i] = NewWriteReq(offset, 4096);
    }
    struct timespec tvs, tve;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    for (size_t i = 0; i < N; i++) {
        ioc_.SendWriteRequest(wreq[i], true);
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    {
        uint64_t used = tdiff(tvs, tve);
        std::cout << "total:" << N
                  << ", SendWriteRequest per:" << used * 1.0f / N << "us"
                  << std::endl;
    }
    uint64_t ps, ds;
    ioc_.GetStats(ds, ps);
    ASSERT_EQ(ds, N);
    UserIoRequest *nextio = NULL;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    for (size_t i = 0; i < N; i++) {
        size_t offset = i * 4096;
        ioc_.OnWriteDone(offset, 4096, &nextio);
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    {
        uint64_t used = tdiff(tvs, tve);
        std::cout << "total:" << N << ", OnWriteDone per:" << used * 1.0f / N
                  << "us" << std::endl;
    }
    ioc_.GetStats(ds, ps);
    ASSERT_EQ(ds, 0);
}

TEST_F(IoConcurrencyTest, TestPerf1k) {
    const int N = 1000;
    UserWriteRequest **wreq = new UserWriteRequest *[N];
    for (size_t i = 0; i < N; i++) {
        size_t offset = i * 4096;
        wreq[i] = NewWriteReq(offset, 4096);
    }
    struct timespec tvs, tve;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    for (size_t i = 0; i < N; i++) {
        ioc_.SendWriteRequest(wreq[i], true);
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    {
        uint64_t used = tdiff(tvs, tve);
        std::cout << "total:" << N
                  << ", SendWriteRequest per:" << used * 1.0f / N << "us"
                  << std::endl;
    }
    uint64_t ps, ds;
    ioc_.GetStats(ds, ps);
    ASSERT_EQ(ds, N);
    UserIoRequest *nextio = NULL;
    clock_gettime(CLOCK_MONOTONIC, &tvs);
    for (size_t i = 0; i < N; i++) {
        size_t offset = i * 4096;
        ioc_.OnWriteDone(offset, 4096, &nextio);
    }
    clock_gettime(CLOCK_MONOTONIC, &tve);
    {
        uint64_t used = tdiff(tvs, tve);
        std::cout << "total:" << N << ", OnWriteDone per:" << used * 1.0f / N
                  << "us" << std::endl;
    }
    ioc_.GetStats(ds, ps);
    ASSERT_EQ(ds, 0);
}
