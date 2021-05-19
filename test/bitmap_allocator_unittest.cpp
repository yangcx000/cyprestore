#include "extentserver/bitmap_allocator.h"

#include "common/status.h"
#include "gtest/gtest.h"

namespace cyprestore {
namespace extentserver {
namespace {

using common::Status;

class BitmapAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        Status s = bitmap_allocator.Init(kBlockSize, kCapacity);
        EXPECT_TRUE(s.ok()) << s.ToString();
    }
    void TearDown() override {}

    const uint64_t kBlockSize = 4ULL << 10;
    const uint64_t kCapacity = (1ULL << 30) + 1024;
    BitmapAllocator bitmap_allocator;
};

TEST_F(BitmapAllocatorTest, TestInit) {
    EXPECT_EQ(bitmap_allocator.NumBlocks(), kCapacity / kBlockSize);
}

TEST_F(BitmapAllocatorTest, TestAllocate) {
    uint64_t num_blocks = bitmap_allocator.NumBlocks();
    Status s;
    int iter = 0;
    while (iter < 2) {
        std::vector<AUnit> aunits;
        uint64_t offset = 0;
        for (uint64_t i = 0; i < num_blocks; ++i) {
            AUnit aunit;
            s = bitmap_allocator.Allocate(kBlockSize, &aunit);
            ASSERT_TRUE(s.ok()) << s.ToString();
            EXPECT_EQ(aunit.offset, offset);
            offset += kBlockSize;
            aunits.push_back(aunit);
        }

        EXPECT_EQ(bitmap_allocator.FreeBlocks(), 0);
        EXPECT_EQ(bitmap_allocator.UsedBlocks(), num_blocks);

        for (size_t i = 0; i < aunits.size(); ++i) {
            const AUnit &aunit = aunits[i];
            bitmap_allocator.Free(aunit.offset, aunit.size);
        }

        EXPECT_EQ(bitmap_allocator.FreeBlocks(), num_blocks);
        EXPECT_EQ(bitmap_allocator.UsedBlocks(), 0);

        ++iter;
    }
}

}  // namespace
}  // namespace extentserver
}  // namespace cyprestore
