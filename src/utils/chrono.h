/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_UTILS_CHRONO_H_
#define CYPRESTORE_UTILS_CHRONO_H_

#include <ctime>
#include <string>

namespace cyprestore {
namespace utils {

class Chrono {
public:
    static std::string GetTimestampStr();
    static std::string DateString();
    static struct timespec CurrentTime();
    static int GetTime(struct timespec *s);
    static uint64_t TimeSinceUs(struct timespec *s, struct timespec *e);
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_CHRONO_H_
