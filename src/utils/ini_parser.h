/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_UTILS_INI_PARSER_H
#define CYPRESTORE_UTILS_INI_PARSER_H

#include <string>

#include "ini_inner.h"

namespace cyprestore {
namespace utils {

class INIParser {
public:
    explicit INIParser(const std::string &conf_path)
            : conf_path_(conf_path), ini_reader_(nullptr) {}

    ~INIParser() {
        delete ini_reader_;
    }

    int Parse();

    bool HasSection(const std::string &section) {
        return ini_reader_->HasSection(section);
    }

    bool HasValue(const std::string &section, const std::string &name) {
        return ini_reader_->HasValue(section, name);
    }

    std::string Get(const std::string &section, const std::string &name);

    std::string GetString(
            const std::string &section, const std::string &name,
            const std::string &default_value);

    bool GetBoolean(
            const std::string &section, const std::string &name,
            bool default_value);

    long GetInteger(
            const std::string &section, const std::string &name,
            long default_value);

    float GetFloat(
            const std::string &section, const std::string &name,
            float default_value);

    double
    GetReal(const std::string &section, const std::string &name,
            double default_value);

private:
    std::string conf_path_;
    INIReader *ini_reader_;
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_INI_PARSER_H
