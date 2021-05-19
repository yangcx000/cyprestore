#include <future>
#include <thread>
#include <vector>

#include "butil/logging.h"
#include "butil/time.h"
#include "common/config.h"
#include "common/cypre_ring.h"
#include "common/status.h"
#include "extentserver/io_mem.h"
#include "extentserver/spdk_mgr.h"
#include "gtest/gtest.h"

namespace cyprestore {
namespace extentserver {
namespace {

using common::Status;

class IOMemTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        InitSpdkEnv();
    }

    static void TearDownTestCase() {
        delete spdk_mgr_;
    }

    void SetUp() override {
        io_mem_ = new IOMem(4096, CypreRing::CypreRingType::TYPE_MP_MC);
        Status s = io_mem_->Init();
        ASSERT_TRUE(s.ok());
    }

    void TearDown() override {
        delete io_mem_;
    }

    static void InitSpdkEnv() {
        const cyprestore::common::SpdkCfg &spdk_cfg =
                cyprestore::GlobalConfig().spdk();
        SpdkEnvOptions env_options;
        env_options.shm_id = spdk_cfg.shm_id;
        env_options.mem_channel = spdk_cfg.mem_channel;
        env_options.mem_size = spdk_cfg.mem_size;
        env_options.master_core = spdk_cfg.master_core;
        env_options.num_pci_addr = spdk_cfg.num_pci_addr;
        env_options.no_pci = spdk_cfg.no_pci;
        env_options.hugepage_single_segments =
                spdk_cfg.hugepage_single_segments;
        env_options.unlink_hugepage = spdk_cfg.unlink_hugepage;
        env_options.core_mask = spdk_cfg.core_mask;
        env_options.huge_dir = spdk_cfg.huge_dir;
        env_options.name = spdk_cfg.name;
        env_options.json_config_file = "bdev.json";
        spdk_mgr_ = new SpdkMgr(env_options);
        auto status = spdk_mgr_->InitEnv();
        if (!status.ok()) {
            LOG(ERROR) << "init spdk env failed";
        }
    }

    IOMem *io_mem_;
    static SpdkMgr *spdk_mgr_;
    int threads_num_ = 300;
    std::vector<std::thread> threads_;

public:
    void run() {
        butil::Timer timer;
        timer.start();
        // uint32_t count = 1024 * 512 - 1;
        uint32_t count = 100;
        while (count > 0) {
            io_u *io = nullptr;
            Status s = io_mem_->GetIOUnitBulk(&io);
            EXPECT_TRUE(s.ok()) << "Count:" << count;
            EXPECT_TRUE(io->data != nullptr);
            auto f = std::async(
                    std::launch::async, &IOMemTest::putIOUnit, this, io);
            --count;
        }
        timer.stop();
        LOG(ERROR) << "time cost: " << timer.u_elapsed() << " us";
    }

    void putIOUnit(io_u *io) {
        // usleep(10000);
        auto s = io_mem_->PutIOUnit(io);
    }
};

SpdkMgr *IOMemTest::spdk_mgr_ = nullptr;
;

TEST_F(IOMemTest, TestGetPutIOUnit) {
    uint32_t count = 1024 * 512;
    std::vector<io_u *> io_vec;
    while (count > 0) {
        io_u *io = nullptr;
        Status s = io_mem_->GetIOUnitBulk(&io);
        ASSERT_TRUE(s.ok()) << "Count:" << count;
        ASSERT_TRUE(io->data != nullptr);
        ASSERT_TRUE(io->size == 4096);
        --count;
        io_vec.push_back(io);
    }
    ASSERT_TRUE(1024 * 512 == io_mem_->used());

    io_u *io = nullptr;
    auto status = io_mem_->GetIOUnitBurst(&io);
    ASSERT_TRUE(!status.ok());

    for (size_t i = 0; i < io_vec.size(); ++i) {
        Status s = io_mem_->PutIOUnit(io_vec[i]);
        EXPECT_TRUE(s.ok()) << "Index:" << i;
    }

    count = 1024 * 512;
    io_vec.clear();
    while (count > 0) {
        io_u *io = nullptr;
        Status s = io_mem_->GetIOUnitBulk(&io);
        ASSERT_TRUE(s.ok()) << "Count:" << count;
        ASSERT_TRUE(io->data != nullptr);
        ASSERT_TRUE(io->size == 4096);
        --count;
        io_vec.push_back(io);
    }
    ASSERT_TRUE(1024 * 512 == io_mem_->used());

    io = nullptr;
    status = io_mem_->GetIOUnitBurst(&io);
    ASSERT_TRUE(!status.ok());

    io_mem_->PutIOUnit(io_vec[0]);
    status = io_mem_->GetIOUnitBurst(&io);
    ASSERT_TRUE(status.ok());
}

TEST_F(IOMemTest, TestMultiGetMultiPutIOUnit) {
    uint32_t count = 1024 * 512;
    uint32_t batch = 2;
    std::vector<io_u *> io_vec;
    while (count != 0) {
        io_u *ios[batch];
        Status s = io_mem_->GetIOUnitsBulk(batch, ios);
        if (!s.ok()) {
            LOG(ERROR) << "error :" << s.ToString();
        }
        ASSERT_TRUE(s.ok()) << "Count:" << count;
        for (uint32_t i = 0; i < batch; i++) {
            io_vec.push_back(ios[i]);
            ASSERT_TRUE(ios[i]->data != nullptr);
            ASSERT_TRUE(ios[i]->size = 4096);
        }
        count -= 2;
    }
    ASSERT_TRUE(1024 * 512 == io_mem_->used());

    io_u *ios[batch];
    auto ret = io_mem_->GetIOUnitsBurst(batch, ios);
    ASSERT_TRUE(ret == 0);

    uint32_t index = 0;
    while (index < 1024 * 512) {
        io_u *ios[2];
        ios[0] = io_vec[index];
        index++;
        ios[1] = io_vec[index];
        index++;
        Status s = io_mem_->PutIOUnits(2, ios);
        EXPECT_TRUE(s.ok()) << "Index: " << index;
    }

    count = 1024 * 512 - 2;
    while (count != 0) {
        io_u *ios[batch];
        Status s = io_mem_->GetIOUnitsBulk(batch, ios);
        ASSERT_TRUE(s.ok()) << "Count:" << count;
        for (uint32_t i = 0; i < batch; i++) {
            ASSERT_TRUE(ios[i]->data != nullptr);
            ASSERT_TRUE(ios[i]->size = 4096);
        }
        count -= 2;
    }

    io_u *ios1[batch + 1];
    ret = io_mem_->GetIOUnitsBurst(batch + 1, ios1);
    ASSERT_TRUE(ret == batch);

    ASSERT_TRUE(1024 * 512 == io_mem_->used());
}

// TEST_F(IOMemTest, TestGetIOUnitPerf) {
//    butil::Timer timer;
//    timer.start();
//    for (int i = 0; i < threads_num_; i++) {
//        threads_.push_back(std::thread(&IOMemTest::run, this));
//    }
//    for (auto &th : threads_) {
//        th.join();
//    }
//    timer.stop();
//    LOG(ERROR) << "all threads run cost: " << timer.u_elapsed() << " us";
//}

}  // namespace
}  // namespace extentserver
}  // namespace cyprestore
