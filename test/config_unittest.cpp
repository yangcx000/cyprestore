#include "common/config.h"

#include "gtest/gtest.h"

namespace cyprestore {
namespace common {
namespace {

TEST(ConfigTest, TestParse) {
    const std::string kModule = "setmanager";
    const std::string kConfigPath = "./sample.ini";

    Config config;
    int ret = config.Init(kModule, kConfigPath);
    ASSERT_EQ(ret, 0);
}

}  // namespace
}  // namespace common
}  // namespace cyprestore
