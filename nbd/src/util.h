/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_UTIL_H_
#define NBD_SRC_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "config.h"

namespace cyprestore {
namespace nbd {

// error code: none free nbd exists
const int CYPRE_ER_NONE_FREE_NBD = -10001;

class NBDListIterator {
public:
    NBDListIterator(bool show_err = false) {
        show_err_ = show_err;
    };

    ~NBDListIterator() = default;
    bool Get(DeviceInfo *pinfo);

private:
    int cur_index_ = 0;
    bool show_err_;
};

// 根据errno打印错误信息，err值无论正负都能处理
extern std::string CppStrerror(int err);

// 从nbd设备名中解析出nbd的index
extern int ParseNbdIndex(const std::string &devpath);

// 获取当前系统能够支持的最大nbd设备数量
extern int GetNbdMaxCount();

// 获取一个当前还未映射的nbd设备名
extern std::string FindUnusedNbdDevice();

// 解析用户输入的命令参数
extern int ParseArgs(
        std::vector<const char *> &args,  // NOLINT
        std::ostream *err_msg, Command *command, NBDConfig *cfg);

int GetMountList(std::map<std::string, std::string> &mount_map);

// return true if devpath is mounted on system
static inline bool IsMounted(const std::string &devpath) {
    std::map<std::string, std::string> mount_map;
    GetMountList(mount_map);
    return mount_map.find(devpath) != mount_map.end();
}

// 获取指定nbd进程对应设备的挂载信息
extern int GetMappedInfo(int pid, NBDConfig *cfg, bool inShowErr = false);

int CheckSizeFromFile(const std::string &path, uint64_t expected_size);

int64_t GetSizeFromFile(const std::string &path);

// 检查指定nbd设备的block size是否符合预期
extern int CheckBlockSize(int nbd_index, uint64_t expected_size);

// 取指定nbd设备的大小
int64_t GetDeviceSize(int nbd_index);

// 检查指定nbd设备的大小是否符合预期
extern int CheckDeviceSize(int nbd_index, uint64_t expected_size);

// 如果当前系统还未加载nbd模块，则进行加载；如果已经加载，则不作任何操作
extern int LoadModule(NBDConfig *cfg);

// 安全读写文件或socket，对异常情况进行处理后返回
ssize_t SafeReadExact(int fd, void *buf, size_t count);

ssize_t SafeRead(int fd, void *buf, size_t count);

ssize_t SafeWrite(int fd, const void *buf, size_t count);

// 网络字节序转换
inline uint64_t Ntohll(uint64_t val) {
    return ((val >> 56) | ((val >> 40) & 0xff00ull)
            | ((val >> 24) & 0xff0000ull) | ((val >> 8) & 0xff000000ull)
            | ((val << 8) & 0xff00000000ull) | ((val << 24) & 0xff0000000000ull)
            | ((val << 40) & 0xff000000000000ull) | ((val << 56)));
}

int NbdErrno(int errcode);

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_UTIL_H_
