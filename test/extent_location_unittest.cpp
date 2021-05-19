#include "gtest/gtest.h"

#define private public
#include "extentserver/extent_location.h"

namespace cyprestore {
namespace extentserver {
namespace {

class ExtentLocationTest : public ::testing::Test {
protected:
    void SetUp() override {
        kvstore::RocksOption rocks_option;
        rocks_option.db_path = db_path_;
        kvstore::RocksStorePtr rocks_store(
                new kvstore::RocksStore(rocks_option));
        auto s = rocks_store->Open();
        ASSERT_TRUE(s.ok());

        extent_loc_mgr_ = new ExtentLocationMgr();
    }
    void TearDown() override {
        delete extent_loc_mgr_;
    }

    const std::string db_path_ = "/tmp/rocksdb_001";
    ExtentLocationMgr *extent_loc_mgr_;
};

TEST_F(ExtentLocationTest, TestStoreExtent) {
    const uint64_t kExtentSize = 1024;
    uint64_t offset = 0, id = 0;
    uint32_t count = 1 << 20;
    while (count > 0) {
        ExtentLocationPtr loc = std::make_shared<ExtentLocation>(
                offset, kExtentSize, std::to_string(id));
        Status s = extent_loc_mgr_->persistExtent(loc);
        EXPECT_TRUE(s.ok());
        offset += kExtentSize;
        id = offset;
        --count;
    }
}

TEST_F(ExtentLocationTest, TestLoadExtents) {
    Status s = extent_loc_mgr_->LoadExtents();
    EXPECT_TRUE(s.ok());
}

}  // namespace
}  // namespace extentserver
}  // namespace cyprestore
