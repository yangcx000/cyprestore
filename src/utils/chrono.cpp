/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "chrono.h"

#include <butil/time/time.h>

#include <cassert>
#include <cstring>
#include <sstream>
#include <string>

namespace cyprestore {
namespace utils {

#define _unused(x) ((void)(x))

std::string Chrono::GetTimestampStr() {
    time_t t;
    t = time(NULL);

    struct tm *tm = localtime(&t);
    assert(tm != nullptr);

    char str[80];
    strftime(str, sizeof(str), "%F.%H-%M-%S", tm);

    return str;
}

std::string Chrono::DateString() {
    butil::Time::Exploded exp;
    butil::Time::Now().LocalExplode(&exp);

    std::stringstream ss;
    ss << exp.year << "-" << exp.month << "-" << exp.day_of_month << "."
       << exp.hour << "-" << exp.minute << "-" << exp.second;

    return ss.str();
}

struct timespec Chrono::CurrentTime() {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now;
}

int Chrono::GetTime(struct timespec *s) {
    return clock_gettime(CLOCK_MONOTONIC, s);
}

uint64_t Chrono::TimeSinceUs(struct timespec *s, struct timespec *e) {
    int64_t sec, usec;

    sec = e->tv_sec - s->tv_sec;
    usec = (e->tv_nsec - s->tv_nsec) / 1000;
    if (sec > 0 && usec < 0) {
        sec--;
        usec += 1000000;
    }

    if (sec < 0 || (sec == 0 && usec < 0)) return 0;

    return usec + (sec * 1000000);
}

}  // namespace utils
}  // namespace cyprestore
