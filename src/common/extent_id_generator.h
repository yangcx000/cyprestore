/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_COMMON_EXTENT_ID_GENERATOR_H_
#define CYPRESTORE_COMMON_EXTENT_ID_GENERATOR_H_

#include <sstream>
#include <string>

namespace cyprestore {
namespace common {

class ExtentIDGenerator {
public:
    static std::string
    GenerateExtentID(const std::string &blob_id, uint64_t extent_index) {
        std::stringstream ss;
        ss << blob_id << "." << extent_index;
        return ss.str();
    }

    static std::string GetBlobId(const std::string &extent_id) {
        auto index = extent_id.find('.');
        return extent_id.substr(0, index);
    }
};

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_EXTENT_ID_GENERATOR_H_
