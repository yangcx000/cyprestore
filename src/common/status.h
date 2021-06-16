/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_COMMON_STATUS_H_
#define CYPRESTORE_COMMON_STATUS_H_

#include <string>

#include "error_code.h"
#include "pb/types.pb.h"

namespace cyprestore {
namespace common {

class Status {
public:
    Status() : code_(CYPRE_OK) {}
    Status(int code) : code_(code) {}
    Status(int code, const std::string &msg) : code_(code), msg_(msg) {}

    static Status OK() {
        return Status(CYPRE_OK);
    }
    static Status InvalidArgument() {
        return Status(CYPRE_ER_INVALID_ARGUMENT);
    }

    bool ok() const {
        return code_ == CYPRE_OK;
    }
    bool IsEmpty() const {
        return code_ == CYPRE_ES_EXTENT_EMPTY;
    }

    int code() const {
        return code_;
    }
    std::string ToString() const {
        return msg_;
    }

private:
    int code_;
    std::string msg_;
};

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_STATUS_H_
