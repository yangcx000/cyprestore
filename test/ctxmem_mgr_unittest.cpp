#include "butil/logging.h"
#include "common/config.h"
#include "common/ctx_mem.h"
#include "common/cypre_ring.h"
#include "common/status.h"
#include "extentserver/spdk_mgr.h"
#include "gtest/gtest.h"

namespace cyprestore {
namespace common {
namespace {

using extentserver::SpdkEnvOptions;
using extentserver::SpdkMgr;

struct ctx_u {
    void *data;
    uint64_t size;
    void *callback;
};

class CtxMemMgrTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        InitSpdkEnv();
    }

    static void TearDownTestCase() {
        delete spdk_mgr_;
    }

    void SetUp() override {
        ctxmem_mgr_ = new CtxMemMgr<ctx_u>(
                "ctx-mem", CypreRing::CypreRingType::TYPE_SP_SC, false);
        Status s = ctxmem_mgr_->Init();
        ASSERT_TRUE(s.ok());
    }

    void TearDown() override {
        delete ctxmem_mgr_;
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
    CtxMemMgr<ctx_u> *ctxmem_mgr_;
};

SpdkMgr *CtxMemMgrTest::spdk_mgr_ = nullptr;
;

TEST_F(CtxMemMgrTest, TestGetPutCtx) {
    ctx_u *ctx = nullptr;
    auto status = ctxmem_mgr_->GetCtxUnitBulk(&ctx);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(1, ctxmem_mgr_->used());

    ctxmem_mgr_->PutCtxUnit(ctx);
    ASSERT_EQ(1, ctxmem_mgr_->used());

    ctx = nullptr;
    status = ctxmem_mgr_->GetCtxUnitBulk(&ctx);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(1, ctxmem_mgr_->used());
}

TEST_F(CtxMemMgrTest, TestMultiGetMultiPutCtx) {
    uint32_t batch = 100;
    ctx_u *ctxs[batch];
    auto status = ctxmem_mgr_->GetCtxUnitsBulk(batch, ctxs);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(batch, ctxmem_mgr_->used());

    ctxmem_mgr_->PutCtxUnits(batch, ctxs);
    ASSERT_EQ(batch, ctxmem_mgr_->used());

    ctx_u *ctxs1[2 * batch];
    status = ctxmem_mgr_->GetCtxUnitsBulk(2 * batch, ctxs1);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(3 * batch, ctxmem_mgr_->used());

    ctxmem_mgr_->PutCtxUnits(2 * batch, ctxs1);
    ASSERT_EQ(3 * batch, ctxmem_mgr_->used());

    ctx_u *ctxs2[3 * batch];
    status = ctxmem_mgr_->GetCtxUnitsBulk(3 * batch, ctxs2);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(3 * batch, ctxmem_mgr_->used());
}

}  // namespace
}  // namespace common
}  // namespace cyprestore
