#include "common/arena.h"

#include "butil/logging.h"
#include "common/config.h"
#include "common/status.h"
#include "extentserver/spdk_mgr.h"
#include "gtest/gtest.h"

namespace cyprestore {
namespace common {
namespace {

using extentserver::SpdkEnvOptions;
using extentserver::SpdkMgr;

class ArenaTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        InitSpdkEnv();
    }

    static void TearDownTestCase() {
        delete spdk_mgr_;
    }

    void SetUp() override {
        ArenaOptions options;
        options.align_size = kAlignSize_;
        options.initial_block_size = kBlockSize_;
        options.max_block_size = kBlockSize_ * 4;
        options.lock_free = false;
        options.use_hugepage = true;
        arena_ = new Arena(options);
    }

    void TearDown() override {
        delete arena_;
    }

    static void InitSpdkEnv() {
        const SpdkCfg &spdk_cfg = cyprestore::GlobalConfig().spdk();
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

    // 1M
    const size_t kBlockSize_ = 1 << 20;
    const size_t kAlignSize_ = 4 << 10;
    const size_t kUnitSize_ = 4 << 10;
    static SpdkMgr *spdk_mgr_;
    Arena *arena_;
};

SpdkMgr *ArenaTest::spdk_mgr_ = nullptr;
;

TEST_F(ArenaTest, TestAllocate) {
    // first allocate 128 * 4K, left 512K
    std::vector<void *> addrs;
    auto status = arena_->Allocate(kUnitSize_, 128, &addrs);
    ASSERT_TRUE(status.ok());

    // allocate 1M, use left 512K and allocate new 2M
    addrs.clear();
    status = arena_->Allocate(kUnitSize_, 256, &addrs);
    ASSERT_TRUE(status.ok());
}

TEST_F(ArenaTest, TestFree) {
    {
        std::vector<void *> addrs;
        auto status = arena_->Allocate(kUnitSize_, 2, &addrs);
        ASSERT_TRUE(status.ok());
        arena_->Release();
    }

    {
        int count = 32768;
        while (count > 0) {
            std::vector<void *> addrs;
            auto status = arena_->Allocate(kUnitSize_, 10, &addrs);
            ASSERT_TRUE(status.ok()) << "Count:" << count;
            --count;
        }
        arena_->Release();
    }
}

}  // namespace
}  // namespace common
}  // namespace cyprestore
