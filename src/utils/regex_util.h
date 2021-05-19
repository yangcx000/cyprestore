/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#ifndef CYPRESTORE_UTILS_REGEX_UTIL_H_
#define CYPRESTORE_UTILS_REGEX_UTIL_H_

#include <string>

namespace cyprestore {
namespace utils {

const std::string USERNAME_PATTERN = "[\\w\\.\\-\\@]+";
const std::string USERID_PATTERN = "user-[\\w\\-]+";
const std::string EMAIL_PATTERN = "[A-Za-z0-9]+\\@jd\\.com";
const std::string BLOBID_PATTERN = "bb-[\\w\\-]+";

class RegexUtil {
public:
    static bool UserNameValid(const std::string &user_name);
    static bool UserIdValid(const std::string &user_id);
    static bool EmailValid(const std::string &email);
    static bool BlobIdValid(const std::string &blob_id);
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_REGEX_UTIL_H_
