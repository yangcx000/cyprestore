#include "extentserver/space_alloc.h"

#include "common/config.h"
#include "common/status.h"
#include "extentserver/block_device.h"
#include "extentserver/nvme_device.h"
#include "gtest/gtest.h"

namespace cyprestore {
namespace extentserver {
namespace {

class SpaceAllocTest : public ::testing::Test {
protected:
    void SetUp() override {
        space_alloc_.reset(new SpaceAlloc(kCapacity));
        int ret = space_alloc_->Init();
        ASSERT_EQ(ret, 0);
    }

    void TearDown() override {}

    const uint64_t kAllocSize = 1ULL << 30;  // 1G
    const uint64_t kCapacity = 4ULL << 40;   // 4T
    SpaceAllocPtr space_alloc_;
};

TEST_F(SpaceAllocTest, TestAllocateAndFree) {
    int count = 4 * 1024;
    std::set<std::unique_ptr<AUnit>> offsets;
    while (count > 0) {
        std::unique_ptr<AUnit> aunit(new AUnit());
        common::Status s = space_alloc_->Allocate(kAllocSize, &aunit);
        ASSERT_TRUE(s.ok());
        if (!s.ok()) {
            return;
        }
        offsets.insert(std::move(aunit));
        --count;
    }

    for (auto it = offsets.begin(); it != offsets.end(); ++it) {
        space_alloc_->Free(&(*it));
    }
}

TEST_F(SpaceAllocTest, TestAllocate) {
    int count = 4 * 1024;
    std::set<std::unique_ptr<AUnit>> offsets;
    while (count > 0) {
        std::unique_ptr<AUnit> aunit(new AUnit());
        common::Status s = space_alloc_->Allocate(kAllocSize, &aunit);
        ASSERT_TRUE(s.ok());
        if (!s.ok()) {
            return;
        }
        offsets.insert(std::move(aunit));
        --count;
    }

    // release one
    int num = 0;
    count = 10;
    uint64_t offset = 1ULL << 40;
    for (auto it = offsets.begin(); it != offsets.end(); ++it) {
        if ((*it)->offset >= offset) {
            continue;
        }
        space_alloc_->Free(&(*it));
        ++it;
        if (++num == count) {
            break;
        }
    }

    while (count > 0) {
        std::unique_ptr<AUnit> aunit(new AUnit());
        common::Status s = space_alloc_->Allocate(kAllocSize, &aunit);
        ASSERT_TRUE(s.ok());
        count--;
    }
}

}  // namespace
}  // namespace extentserver
}  // namespace cyprestore
