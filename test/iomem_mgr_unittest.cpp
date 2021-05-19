#include "butil/logging.h"
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

class IOMemMgrTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        InitSpdkEnv();
    }

    static void TearDownTestCase() {
        delete spdk_mgr_;
    }

    void SetUp() override {
        iomem_mgr_ = new IOMemMgr(CypreRing::CypreRingType::TYPE_SP_SC);
        Status s = iomem_mgr_->Init();
        ASSERT_TRUE(s.ok());
    }

    void TearDown() override {
        delete iomem_mgr_;
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

    static SpdkMgr *spdk_mgr_;
    IOMemMgr *iomem_mgr_;
};

SpdkMgr *IOMemMgrTest::spdk_mgr_ = nullptr;
;

TEST_F(IOMemMgrTest, TestGetPutIOUnit) {
    const uint32_t kIOUnit = 4096;
    std::vector<io_u *> ios;
    for (int i = 1; i <= 256; ++i) {
        io_u *io;
        Status s = iomem_mgr_->GetIOUnitBulk(kIOUnit * i, &io);
        ASSERT_TRUE(s.ok()) << "Index:" << i;
        ASSERT_EQ(io->size, kIOUnit * i);
        ios.push_back(io);
    }
    io_u *io;
    auto s = iomem_mgr_->GetIOUnitBulk(kIOUnit * 257, &io);
    ASSERT_TRUE(s.ok());

    s = iomem_mgr_->GetIOUnitBurst(kIOUnit, &io);
    ASSERT_TRUE(!s.ok());

    for (auto &io : ios) {
        iomem_mgr_->PutIOUnit(io);
    }

    s = iomem_mgr_->GetIOUnitBurst(kIOUnit, &io);
    ASSERT_TRUE(s.ok());

    uint32_t unit_size = 1 << 21;  // 2M
    uint32_t max_size = 10 << 21;  // 20M
    while (unit_size <= max_size) {
        io_u *io;
        Status s = iomem_mgr_->GetIOUnitBulk(unit_size, &io);
        ASSERT_TRUE(s.ok());
        ASSERT_EQ(io->size, unit_size);

        iomem_mgr_->PutIOUnit(io);
        unit_size += 1 << 20;
    }

    s = iomem_mgr_->GetIOUnitBurst(unit_size, &io);
    ASSERT_TRUE(!s.ok());
}

TEST_F(IOMemMgrTest, TestMultiGetMultiPutIOUnit) {
    const uint32_t kIOUnit = 4096;
    uint32_t batch = 2;
    for (int i = 1; i <= 256; ++i) {
        io_u *ios[batch];
        Status s = iomem_mgr_->GetIOUnitsBulk(kIOUnit * i, batch, ios);
        ASSERT_TRUE(s.ok());
        for (uint32_t index = 0; index < batch; index++) {
            ASSERT_EQ(ios[index]->size, kIOUnit * i);
            ASSERT_TRUE(ios[index]->data != nullptr);
        }

        iomem_mgr_->PutIOUnits(batch, ios);
    }

    io_u *ios2[batch];
    auto s = iomem_mgr_->GetIOUnitsBulk(kIOUnit * 257, batch, ios2);
    ASSERT_TRUE(s.ok());

    io_u *ios1[batch + 1];
    auto ret = iomem_mgr_->GetIOUnitsBurst(kIOUnit, batch + 1, ios1);
    ASSERT_TRUE(ret == batch);

    uint32_t unit_size = 1 << 21;  // 2M
    uint32_t max_size = 10 << 20;  // 10M
    while (unit_size <= max_size) {
        io_u *ios[batch];
        Status s = iomem_mgr_->GetIOUnitsBulk(unit_size, batch, ios);
        ASSERT_TRUE(s.ok());
        for (uint32_t i = 0; i < batch; i++) {
            ASSERT_EQ(ios[i]->size, unit_size);
            ASSERT_TRUE(ios[i]->data != nullptr);
        }
        iomem_mgr_->PutIOUnits(batch, ios);
        unit_size += 1 << 20;
    }

    ret = iomem_mgr_->GetIOUnitsBurst(unit_size, batch + 1, ios1);
    ASSERT_TRUE(ret == 0);
}

}  // namespace
}  // namespace extentserver
}  // namespace cyprestore
