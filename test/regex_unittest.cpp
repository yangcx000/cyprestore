/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "gtest/gtest.h"
#include "utils/regex_util.h"

namespace cyprestore {
namespace utils {

class RegexUtilTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(RegexUtilTest, TestUserName) {
    std::string user_name = "asdfkljsdflkj124i-_@";
    ASSERT_TRUE(RegexUtil::UserNameValid(user_name));

    user_name = "dafdsfdasfaf>";
    ASSERT_FALSE(RegexUtil::UserNameValid(user_name));
}

TEST_F(RegexUtilTest, TestUserId) {
    std::string user_id = "user-sadfadfsa";
    ASSERT_TRUE(RegexUtil::UserIdValid(user_id));

    user_id = "userr-sdfadf";
    ASSERT_FALSE(RegexUtil::UserIdValid(user_id));
}

TEST_F(RegexUtilTest, TestEmail) {
    std::string email = "test45@jd.com";
    ASSERT_TRUE(RegexUtil::EmailValid(email));

    email = "userr-sdfadf@jd..com";
    ASSERT_FALSE(RegexUtil::EmailValid(email));
}

TEST_F(RegexUtilTest, TestBlobId) {
    std::string blob_id = "bb-sadfadfsa";
    ASSERT_TRUE(RegexUtil::BlobIdValid(blob_id));

    blob_id = "bbb-sdfadf";
    ASSERT_FALSE(RegexUtil::BlobIdValid(blob_id));
}

}  // namespace utils
}  // namespace cyprestore
