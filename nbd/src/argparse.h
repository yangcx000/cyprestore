/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_ARGPARSE_H_
#define NBD_SRC_ARGPARSE_H_

#include <stdarg.h>

#include <string>
#include <vector>

namespace cyprestore {
namespace nbd {

float strict_strtof(const char *str, std::string *err);
int64_t strict_strtoll(const char *str, int base, std::string *err);
int strict_strtol(const char *str, int base, std::string *err);
void arg_value_type(
        const char *nextargstr, bool *bool_option, bool *bool_numeric);
// 将二维数组转成vector形式
void argv_to_vec(
        int argc, const char **argv,
        std::vector<const char *> &args);  // NOLINT
/**
 * @brief:
 * 解析参数，如果指定位置的参数内容符合预期的值，则返回true，否则返回false
 * @param args: 存放参数的向量
 * @param i: 起始的向量迭代器
 * @param ...: 预期的参数名，可以指定多个
 * @return 符合预期值返回true，否则返回false
 */
bool argparse_flag(
        std::vector<const char *> &args,               // NOLINT
        std::vector<const char *>::iterator &i, ...);  // NOLINT
/**
 * @brief:
 * 解析参数，如果指定位置的参数内容符合预期的值，则返回true，否则返回false
 * 该方法解析的参数对应参数值
 * 例如 --device /dev/nbd0，--device为参数，/dev/nbd0为参数值
 * @param args: 存放参数的向量
 * @param i: 起始的向量迭代器
 * @param ret: 参数值，该方法为string 模板特化
 * @param oss: 记录错误信息
 * @param ...: 预期的参数名，可以指定多个
 * @return 符合预期值返回true，否则返回false
 */
bool argparse_witharg(
        std::vector<const char *> &args,  // NOLINT
        std::vector<const char *>::iterator &i,
        std::string *ret,         // NOLINT
        std::ostream &oss, ...);  // NOLINT
/**
 * @brief:
 * 解析参数，如果指定位置的参数内容符合预期的值，则返回true，否则返回false
 * 该方法解析的参数对应参数值
 * 例如 --device /dev/nbd0，--device为参数，/dev/nbd0为参数值
 * @param args: 存放参数的向量
 * @param i: 起始的向量迭代器
 * @param ret: 参数值
 * @param oss: 记录错误信息
 * @param ...: 预期的参数名，可以指定多个
 * @return 符合预期值返回true，否则返回false
 */
template <class T>
bool argparse_witharg(
        std::vector<const char *> &args,                 // NOLINT
        std::vector<const char *>::iterator &i, T *ret,  // NOLINT
        std::ostream &oss, ...);                         // NOLINT

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_ARGPARSE_H_
