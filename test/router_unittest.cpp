/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "extentmanager/replication_group.h"
#include "gtest/gtest.h"
#include "utils/chrono.h"

namespace cyprestore {
namespace extentmanager {
namespace {

class RouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto rg1 = std::make_shared<ReplicationGroup>("rg-1", "pool-1");
        auto rg2 = std::make_shared<ReplicationGroup>("rg-2", "pool-1");
        auto rg3 = std::make_shared<ReplicationGroup>("rg-3", "pool-1");
        auto rg4 = std::make_shared<ReplicationGroup>("rg-4", "pool-1");
        auto rg5 = std::make_shared<ReplicationGroup>("rg-5", "pool-1");
        auto rg6 = std::make_shared<ReplicationGroup>("rg-6", "pool-1");
        rg_heap_.push(rg1);
        rg_heap_.push(rg2);
        rg_heap_.push(rg3);
        rg_heap_.push(rg4);
        rg_heap_.push(rg5);
        rg_heap_.push(rg6);
    }

    void TearDown() override {}

    RGHeap rg_heap_;
    int num_ = 300;
    int rg_num_ = 6;
    std::map<std::string, int> rg_nr_map_;
};

TEST_F(RouterTest, TestAllocateRG) {
    for (int i = 0; i < num_; i++) {
        auto rg = rg_heap_.top();
        rg->add_nr_extents();
        rg_heap_.pop();
        rg_heap_.push(rg);
        rg_nr_map_[rg->kv_key()]++;
    }

    for (auto &m : rg_nr_map_) {
        ASSERT_EQ(num_ / rg_num_, m.second);
    }
}

}  // namespace
}  // namespace extentmanager
}  // namespace cyprestore
