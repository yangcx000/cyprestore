/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_UTILS_CODING_H_
#define CYPRESTORE_UTILS_CODING_H_

#include <string>

namespace cyprestore {
namespace utils {

class Coding {
public:
    static void PutFixed32(std::string *dst, uint32_t value);
    static bool GetFixed32(std::string *input, uint32_t *value);
    static uint32_t DecodeFixed32(const char *ptr);
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_CODING_H_
