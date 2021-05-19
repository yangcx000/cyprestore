/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "ini_parser.h"

#include <cassert>  // assert

namespace cyprestore {
namespace utils {

int INIParser::Parse() {
    ini_reader_ = new INIReader(conf_path_);
    assert(ini_reader_ && "Can't construct ini_reader");
    return ini_reader_->ParseError();
}

std::string
INIParser::Get(const std::string &section, const std::string &name) {
    const std::string &value = ini_reader_->Get(section, name, "");
    assert(!value.empty() && "Missing value");

    // 递归终止
    if (value.find('@') == std::string::npos) return value;

    // 校验value是否合法
    assert(std::count(value.begin(), value.end(), '{')
                   == std::count(value.begin(), value.end(), '}')
           && "{ not match }");

    std::string ret;
    size_t pos = 0, found = value.find('@', pos);
    while (found != std::string::npos) {
        ret.append(value.begin() + pos, value.begin() + found);
        // 解析{}
        size_t left = value.find('{', found);
        size_t right = value.find('}', found);
        assert(left != std::string::npos && right != std::string::npos
               && "invalid { and }");

        std::string key = value.substr(left + 1, right - left - 1);
        size_t dot = key.find('.', 0);
        assert(dot != std::string::npos && "invalid section.name");
        std::string section_value = key.substr(0, dot);
        std::string name_value = key.substr(dot + 1, key.size() - dot - 1);
        ret.append(Get(section_value, name_value));

        pos = right + 1;
        found = value.find('@', pos);
    }

    ret.append(value.begin() + pos, value.end());
    return ret;
}

std::string INIParser::GetString(
        const std::string &section, const std::string &name,
        const std::string &default_value) {
    if (!HasValue(section, name)) return default_value;

    return Get(section, name);
}

bool INIParser::GetBoolean(
        const std::string &section, const std::string &name,
        bool default_value) {
    if (!HasValue(section, name)) return default_value;

    return ini_reader_->GetBoolean(section, name, default_value);
}

long INIParser::GetInteger(
        const std::string &section, const std::string &name,
        long default_value) {
    if (!HasValue(section, name)) return default_value;

    return ini_reader_->GetInteger(section, name, default_value);
}

float INIParser::GetFloat(
        const std::string &section, const std::string &name,
        float default_value) {
    if (!HasValue(section, name)) return default_value;

    return ini_reader_->GetFloat(section, name, default_value);
}

double INIParser::GetReal(
        const std::string &section, const std::string &name,
        double default_value) {
    if (!HasValue(section, name)) return default_value;

    return ini_reader_->GetReal(section, name, default_value);
}

}  // namespace utils
}  // namespace cyprestore
