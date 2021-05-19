/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "regex_util.h"

#include <boost/regex.hpp>

namespace cyprestore {
namespace utils {

bool RegexUtil::UserNameValid(const std::string &user_name) {
    boost::regex rg(USERNAME_PATTERN);
    return boost::regex_match(user_name, rg);
}

bool RegexUtil::UserIdValid(const std::string &user_id) {
    boost::regex rg(USERID_PATTERN);
    return boost::regex_match(user_id, rg);
}

bool RegexUtil::EmailValid(const std::string &email) {
    boost::regex rg(EMAIL_PATTERN);
    return boost::regex_match(email, rg);
}

bool RegexUtil::BlobIdValid(const std::string &blob_id) {
    boost::regex rg(BLOBID_PATTERN);
    return boost::regex_match(blob_id, rg);
}

}  // namespace utils
}  // namespace cyprestore
