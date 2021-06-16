/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_UTILS_SERIALIZER_H_
#define CYPRESTORE_UTILS_SERIALIZER_H_

#include <cereal/archives/binary.hpp>
#include <sstream>
#include <string>

#include "butil/logging.h"
#include "utils/coding.h"
#include "utils/crc32.h"

namespace cyprestore {
namespace utils {

template <class T> class Serializer {
public:
    static std::string Encode(const T &input);
    static bool Decode(const std::string &data, T &output);
};

template <class T> std::string Serializer<T>::Encode(const T &input) {
    std::stringstream ss;
    {
        cereal::BinaryOutputArchive oarchive(ss);
        oarchive(input);
    }

    std::string output = ss.str();
    uint32_t crc32_value = Crc32::Checksum(output);
    Coding::PutFixed32(&output, crc32_value);
    return output;
}

template <class T>
bool Serializer<T>::Decode(const std::string &input, T &output) {
    uint32_t crc32 = 0;
    std::string data = input;
    bool ret = Coding::GetFixed32(&data, &crc32);
    if (!ret) {
        LOG(ERROR) << "Decode could't get crc32";
        return ret;
    }

    uint32_t expected_crc32 = Crc32::Checksum(data);
    if (crc32 != expected_crc32) {
        LOG(ERROR) << "Decode get crc32 value mismatch"
                   << ", crc32:" << crc32
                   << ", expected_crc32:" << expected_crc32;
        return false;
    }

    std::stringstream ss(data);
    {
        cereal::BinaryInputArchive iarchive(ss);
        iarchive(output);
    }
    return true;
}

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_SERIALIZER_H_
