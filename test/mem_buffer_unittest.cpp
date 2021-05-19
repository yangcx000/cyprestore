#include "common/mem_buffer.h"

#include "butil/logging.h"
#include "common/arena.h"
#include "common/config.h"
#include "common/cypre_ring.h"
#include "common/status.h"
#include "extentserver/spdk_mgr.h"
#include "gtest/gtest.h"

namespace cyprestore {
namespace common {
namespace {

using extentserver::SpdkEnvOptions;
using extentserver::SpdkMgr;

struct io_u {
    void *data;
    uint64_t size;
};

class MemBufferTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        InitSpdkEnv();
    }

    static void TearDownTestCase() {
        delete spdk_mgr_;
    }

    void SetUp() override {
        mem_buffer_ = new MemBuffer<io_u>(
                "mem-buffer", CypreRing::CypreRingType::TYPE_SP_SC, ring_size_,
                true);
        auto status = mem_buffer_->init();
        if (!status.ok()) {
            LOG(ERROR) << status.ToString();
        }
        ASSERT_TRUE(status.ok());
    }

    void TearDown() override {
        delete mem_buffer_;
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

    static SpdkMgr *spdk_mgr_;
    MemBuffer<io_u> *mem_buffer_;
    uint32_t ring_size_ = 10000;
};

SpdkMgr *MemBufferTest::spdk_mgr_ = nullptr;
;

TEST_F(MemBufferTest, TestGet) {
    io_u *io = nullptr;
    auto status = mem_buffer_->GetBulk(&io);
    ASSERT_TRUE(status.ok());

    io = nullptr;
    status = mem_buffer_->GetBurst(&io);
    ASSERT_TRUE(!status.ok());
}

TEST_F(MemBufferTest, TestPut) {
    io_u *io = nullptr;
    auto status = mem_buffer_->GetBulk(&io);
    ASSERT_TRUE(status.ok());

    status = mem_buffer_->Put(io);
    ASSERT_TRUE(status.ok());

    io = nullptr;
    status = mem_buffer_->GetBurst(&io);
    ASSERT_TRUE(status.ok());
}

TEST_F(MemBufferTest, TestMultiGet) {
    uint32_t num = 100;
    io_u *addrs[num];
    auto status = mem_buffer_->GetsBulk(num, addrs);
    ASSERT_TRUE(status.ok());

    io_u *addrs1[num];
    auto ret = mem_buffer_->GetsBurst(num, addrs1);
    ASSERT_TRUE(ret == 0);
}

TEST_F(MemBufferTest, TestMultiPut) {
    uint32_t num = 100;
    io_u *addrs[num];
    auto status = mem_buffer_->GetsBulk(num, addrs);
    ASSERT_TRUE(status.ok());

    status = mem_buffer_->Puts(num, addrs);
    ASSERT_TRUE(status.ok());

    io_u *addrs1[num + 1];
    auto ret = mem_buffer_->GetsBurst(num + 1, addrs1);
    ASSERT_TRUE(ret == num);
}

}  // namespace
}  // namespace common
}  // namespace cyprestore
