#include "utils/ini_parser.h"

#include <string>

#include "gtest/gtest.h"

namespace cyprestore {
namespace utils {
namespace {

class INIParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        ini_parser_ = new INIParser(kINIConfPath);
        ini_parser_->Parse();
    }

    void TearDown() override {
        delete ini_parser_;
    }

    const std::string kINIConfPath = "./sample.ini";
    INIParser *ini_parser_;
};

TEST_F(INIParserTest, TestMethodParse) {
    ASSERT_EQ(ini_parser_->Parse(), 0);
}

TEST_F(INIParserTest, TestMethodGetString) {
    std::string value;

    value = ini_parser_->GetString("common", "subsys", "");
    EXPECT_EQ(value, "cyprestore");

    value = ini_parser_->GetString("common", "instance", "");
    EXPECT_EQ(value, "1");

    value = ini_parser_->GetString("common", "myname", "");
    EXPECT_EQ(value, "/NS/cyprestore/yz/setmanager/1");

    value = ini_parser_->GetString("log", "logpath", "");
    EXPECT_EQ(value, "/var/log/cyprestore/setmanager");

    value = ini_parser_->GetString("log", "prefix", "");
    EXPECT_EQ(value, "setmanager_1_");

    value = ini_parser_->GetString("testcase-string", "test1", "");
    EXPECT_EQ(value, "/NS/cyprestore/yz/setmanager/1");

    value = ini_parser_->GetString("testcase-string", "test2", "");
    EXPECT_EQ(value, "/beijing");

    value = ini_parser_->GetString("testcase-string", "test3", "");
    EXPECT_EQ(value, "beijing/");

    value = ini_parser_->GetString("testcase-string", "test4", "");
    EXPECT_EQ(value, "cyprestoresetmanager");
}

TEST_F(INIParserTest, TestMethodGetInteger) {
    long value = ini_parser_->GetInteger("common", "instance", -1);
    EXPECT_EQ(value, 1);

    value = ini_parser_->GetInteger("network", "public_port", -1);
    EXPECT_EQ(value, 9100);
}

TEST_F(INIParserTest, TestMethodGetBoolean) {
    bool value = ini_parser_->GetBoolean("testcase-boolean", "test1", false);
    EXPECT_TRUE(value);

    value = ini_parser_->GetBoolean("testcase-boolean", "test2", false);
    EXPECT_FALSE(value);
}

TEST_F(INIParserTest, TestMethodGetReal) {
    double value = ini_parser_->GetReal("testcase-real", "test1", false);
    EXPECT_EQ(value, (double)6.000);

    value = ini_parser_->GetReal("testcase-real", "test2", false);
    EXPECT_EQ(value, (double)0.6);
}

TEST_F(INIParserTest, TestMethodGetFloat) {
    float value = ini_parser_->GetFloat("testcase-float", "test1", false);
    EXPECT_EQ(value, (float)3.14);

    value = ini_parser_->GetFloat("testcase-float", "test2", false);
    EXPECT_EQ(value, (float)3.0);
}

}  // namespace
}  // namespace utils
}  // namespace cyprestore
