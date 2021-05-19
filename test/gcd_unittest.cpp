/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "gtest/gtest.h"
#include "utils/gcd_util.h"

namespace cyprestore {
namespace utils {

class GcdUtilTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(GcdUtilTest, TestGcd) {
    std::vector<int> nums;
    nums.push_back(4);
    nums.push_back(10);
    nums.push_back(16);
    nums.push_back(14);
    ASSERT_EQ(2, GCDUtil::gcd(nums));
}

}  // namespace utils
}  // namespace cyprestore
