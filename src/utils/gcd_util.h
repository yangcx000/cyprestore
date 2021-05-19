/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#ifndef CYPRESTORE_UTILS_GCDUTIL_H_
#define CYPRESTORE_UTILS_GCDUTIL_H_

#include <vector>

namespace cyprestore {
namespace utils {

class GCDUtil {
public:
    static int gcd(const std::vector<int> &nums);
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_GCDUTIL_H_
