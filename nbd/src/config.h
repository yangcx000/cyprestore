/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_DEFINE_H_
#define NBD_SRC_DEFINE_H_

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace cyprestore {
namespace nbd {

#define HELP_INFO 1
#define VERSION_INFO 2
#define NBD_BLKSIZE 4096UL  // 后端当前支持4096大小对齐的IO

#define NBD_MAX_PATH "/sys/module/nbd/parameters/nbds_max"
#define PROCESS_NAME "cypre_nbd"
#define NBD_PATH_PREFIX "/sys/block/nbd"
#define DEV_PATH_PREFIX "/dev/nbd"

struct NBDConfig {
    // 设置系统提供的nbd设备的数量
    int nbds_max = 0;
    // 设置一个nbd设备能够支持的最大分区数量
    int max_part = 255;
    // 通过测试，nbd默认的io超时时间好像是3s，这里默认将值设置为5分钟
    int timeout = 300;
    // 设置nbd设备是否为只读
    bool readonly = false;
    // 是否需要设置最大分区数量
    bool set_max_part = false;
    // 是否以netlink方式控制nbd内核模块
    bool try_netlink = false;
    // 需要映射的后端文件名称
    std::string imgname;
    // 指定需要映射的nbd设备路径
    std::string devpath;

    int multi_conns = 1;
    bool debug = false;
    bool foreground = false;
    bool nullio = false;
    std::string em_endpoint;  // ip:port
    int dummy_port = 0;
    std::string client_core_mask;
    std::string logpath;
};

// 用户命令类型
enum class Command { None, Connect, Disconnect, List };

struct DeviceInfo {
    int pid;
    int64_t size;
    std::string devpath;
    std::string blob_id;
    std::string mnt_point;
};

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_DEFINE_H_
