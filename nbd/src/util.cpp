/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#include "util.h"

#include <arpa/inet.h>
#include <butil/logging.h>
#include <errno.h>
#include <libgen.h>
#include <linux/nbd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <sstream>

#include "argparse.h"
#include "common/error_code.h"
#include "config.h"
#include "plus.h"

namespace cyprestore {
namespace nbd {

std::string CppStrerror(int err) {
    if (err > 0) err = -err;
    switch (err) {
        case CYPRE_ER_NONE_FREE_NBD:
            return "NONE_FREE_NBD";
        // common
        case common::CYPRE_ER_INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";
        case common::CYPRE_ER_NO_PERMISSION:
            return "NO PERMISSION";
        case common::CYPRE_ER_NOT_SUPPORTED:
            return "NOT_SUPPORTED";
        case common::CYPRE_ER_TIMEDOUT:
            return "TIMEDOUT";
        case common::CYPRE_ER_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case common::CYPRE_ER_NET_ERROR:
            return "NET_ERROR";
        // client sdk
        case common::CYPRE_C_STREAM_CORRUPT:
            return "STREAM_CORRUPT";
        case common::CYPRE_C_DATA_TOO_LARGE:
            return "DATA_TOO_LARGE";
        case common::CYPRE_C_DEVICE_CLOSED:
            return "DEVICE_CLOSED";
        case common::CYPRE_C_INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        case common::CYPRE_C_HANDLE_NOT_FOUND:
            return "HANDLE_NOT_FOUND";
        case common::CYPRE_C_RING_FULL:
            return "RING_FULL";
        case common::CYPRE_C_RING_EMPTY:
            return "RING_EMPTY";
        case common::CYPRE_C_CRC_ERROR:
            return "CRC_ERROR";
        // ExtentManager
        case common::CYPRE_EM_POOL_NOT_FOUND:
            return "POOL_NOT_FOUND";
        case common::CYPRE_EM_BLOB_NOT_FOUND:
            return "BLOB_NOT_FOUND";
        case common::CYPRE_EM_USER_NOT_FOUND:
            return "USER_NOT_FOUND";
        case common::CYPRE_EM_ROUTER_NOT_FOUND:
            return "ROUTER_NOT_FOUND";
        case common::CYPRE_EM_ES_NOT_FOUND:
            return "ES_NOT_FOUND";
        case common::CYPRE_EM_STORE_ERROR:
            return "STORE_ERROR";
        case common::CYPRE_EM_LOAD_ERROR:
            return "LOAD_ERROR";
        case common::CYPRE_EM_DELETE_ERROR:
            return "DELETE_ERROR";
        case common::CYPRE_EM_DECODE_ERROR:
            return "DECODE_ERROR";
        case common::CYPRE_EM_ENCODE_ERROR:
            return "ENCODE_ERROR";
        case common::CYPRE_EM_ES_DUPLICATED:
            return "ES_DUPLICATED";
        case common::CYPRE_EM_POOL_ALREADY_INIT:
            return "POOL_ALREADY_INIT";
        case common::CYPRE_EM_POOL_NOT_READY:
            return "POOL_NOT_READY";
        case common::CYPRE_EM_POOL_DUPLICATED:
            return "POOL_DUPLICATED";
        case common::CYPRE_EM_POOL_ALLOCATE_FAIL:
            return "POOL_ALLOCATE_FAIL";
        case common::CYPRE_EM_RG_ALLOCATE_FAIL:
            return "RG_ALLOCATE_FAIL";
        case common::CYPRE_EM_RG_NOT_FOUND:
            return "RG_NOT_FOUND";
        case common::CYPRE_EM_USER_DUPLICATED:
            return "USER_DUPLICATED";
        case common::CYPRE_EM_EXCEED_TIME_LIMIT:
            return "EXCEED_TIME_LIMIT";
        case common::CYPRE_EM_TRY_AGAIN:
            return "TRY_AGAIN";
        case common::CYPRE_EM_GET_CONN_ERROR:
            return "GET_CONN_ERROR";
        case common::CYPRE_EM_BRPC_SEND_FAIL:
            return "BRPC_SEND_FAIL";
    }
    char buf[128];
    if (err < 0) err = -err;
    std::ostringstream oss;
    oss << strerror_r(err, buf, sizeof(buf));
    return oss.str();
}

int ParseNbdIndex(const std::string &devpath) {
    int index, ret;

    ret = sscanf(devpath.c_str(), "/dev/nbd%d", &index);
    if (ret <= 0) {
        // mean an early matching failure. But some cases need a negative value.
        if (ret == 0) {
            ret = -EINVAL;
        }
        std::cerr << "Error: invalid device path: " << devpath
                  << " (expected /dev/nbd{num})" << std::endl;
        return ret;
    }

    return index;
}

int GetNbdMaxCount() {
    int nbds_max = -1;
    if (access(NBD_MAX_PATH, F_OK) == 0) {
        std::ifstream ifs;
        ifs.open(NBD_MAX_PATH, std::ifstream::in);
        if (ifs.is_open()) {
            ifs >> nbds_max;
            ifs.close();
        }
    }
    return nbds_max;
}

std::string FindUnusedNbdDevice() {
    int index = 0;
    int devfd = 0;
    int nbds_max = GetNbdMaxCount();
    char dev[64];
    int sockfd[2];

    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd);
    if (ret < 0) {
        LOG(ERROR) << " failed to create socket pair.";
        return "";
    }

    while (true) {
        std::string nbd_pid_path =
                NBD_PATH_PREFIX + std::to_string(index) + "/pid";
        if (::access(nbd_pid_path.c_str(), F_OK) == 0) {
            ++index;
            continue;
        }

        snprintf(dev, sizeof(dev), "/dev/nbd%d", index);

        ret = open(dev, O_RDWR);
        if (ret < 0) {
            if (ret == -EPERM && nbds_max != -1 && index < (nbds_max - 1)) {
                ++index;
                continue;
            }
            LOG(ERROR) << " failed to find unused device, "
                       << CppStrerror(errno);
            break;
        }

        devfd = ret;
        ret = ioctl(devfd, NBD_SET_SOCK, sockfd[0]);
        if (ret < 0) {
            close(devfd);
            ++index;
            continue;
        }
        break;
    }

    std::string result = "";
    if (ret == 0) {
        result = dev;
        ioctl(devfd, NBD_CLEAR_SOCK);
        close(devfd);
    }
    close(sockfd[0]);
    close(sockfd[1]);
    return result;
}

int ParseArgs(
        std::vector<const char *> &args, std::ostream *err_msg,  // NOLINT
        Command *command, NBDConfig *cfg) {
    std::vector<const char *>::iterator i;
    std::ostringstream err;
    for (i = args.begin(); i != args.end();) {

        // printf("args: %s\r\n",(*i));
        if (argparse_flag(args, i, "-h", "-help", (char *)NULL)) {  // NOLINT
            return HELP_INFO;
        } else if (argparse_flag(
                           args, i, "-v", "-version",
                           (char *)NULL)) {  // NOLINT
            return VERSION_INFO;
        } else if (argparse_witharg(
                           args, i, &cfg->devpath, err, "-device",
                           (char *)NULL)) {                           // NOLINT
        } else if (argparse_flag(args, i, "-debug", (char *)NULL)) {  // NOLINT
            cfg->debug = true;
        } else if (argparse_flag(args, i, "-f", (char *)NULL)) {  // NOLINT
            cfg->foreground = true;
        } else if (argparse_flag(args, i, "-nullio", (char *)NULL)) {  // NOLINT
            cfg->nullio = true;
        } else if (argparse_witharg(
                           args, i, &cfg->em_endpoint, err, "-em_endpoint",
                           (char *)NULL)) {  // NOLINT
        } else if (argparse_witharg(
                           args, i, &cfg->client_core_mask, err,
                           "-client_core_mask",
                           (char *)NULL)) {  // NOLINT
        } else if (argparse_witharg(
                           args, i, &cfg->multi_conns, err, "-multi_conns",
                           (char *)NULL)) {  // NOLINT
            if (!err.str().empty()) {
                *err_msg << "cypre_ndb: " << err.str();
                return -EINVAL;
            }
            if (cfg->multi_conns < 1) {
                *err_msg << "cypre_ndb: Invalid argument for multi_conns!";
                return -EINVAL;
            }
        } else if (argparse_witharg(
                           args, i, &cfg->dummy_port, err, "-dummy_port",
                           (char *)NULL)) {
            if (!err.str().empty()) {
                *err_msg << "cypre_ndb: " << err.str();
                return -EINVAL;
            }
        } else if (argparse_witharg(
                           args, i, &cfg->nbds_max, err, "-nbds_max",
                           (char *)NULL)) {  // NOLINT
            if (!err.str().empty()) {
                *err_msg << "cypre_ndb: " << err.str();
                return -EINVAL;
            }
            if (cfg->nbds_max < 0) {
                *err_msg << "cypre_ndb: Invalid argument for nbds_max!";
                return -EINVAL;
            }
        } else if (argparse_witharg(
                           args, i, &cfg->max_part, err, "-max_part",
                           (char *)NULL)) {  // NOLINT
            if (!err.str().empty()) {
                *err_msg << "cypre_ndb: " << err.str();
                return -EINVAL;
            }
            if ((cfg->max_part < 0) || (cfg->max_part > 255)) {
                *err_msg << "cypre_ndb: Invalid argument for max_part(0~255)!";
                return -EINVAL;
            }
            cfg->set_max_part = true;
        } else if (argparse_flag(
                           args, i, "-readonly", (char *)NULL)) {  // NOLINT
            cfg->readonly = true;
        } else if (argparse_witharg(
                           args, i, &cfg->timeout, err, "-timeout",
                           (char *)NULL)) {  // NOLINT
            if (!err.str().empty()) {
                *err_msg << "cypre_ndb: " << err.str();
                return -EINVAL;
            }
            if (cfg->timeout < 0) {
                *err_msg << "cypre_ndb: Invalid argument for timeout!";
                return -EINVAL;
            }
        } else if (argparse_flag(
                           args, i, "-try_netlink", (char *)NULL)) {  // NOLINT
            // TODO: cfg->try_netlink = true;
        } else if (argparse_witharg(
                           args, i, &cfg->logpath, err, "-logpath",
                           (char *)NULL)) {  // NOLINT
        } else {
            ++i;
        }
    }

    Command cmd = Command::None;
    if (args.begin() != args.end()) {
        if (strcmp(*args.begin(), "map") == 0) {
            cmd = Command::Connect;
        } else if (strcmp(*args.begin(), "unmap") == 0) {
            cmd = Command::Disconnect;
        } else if (strcmp(*args.begin(), "list") == 0) {
            cmd = Command::List;
        } else {
            *err_msg << "cypre_ndb: unknown command: " << *args.begin();
            return -EINVAL;
        }
        args.erase(args.begin());
    }

    if (cmd == Command::None) {
        *err_msg << "cypre_ndb: must specify command";
        return -EINVAL;
    }

    switch (cmd) {
        case Command::Connect:
            if (args.begin() == args.end()) {
                *err_msg << "cypre_ndb: must specify image spec";
                return -EINVAL;
            }
            cfg->imgname = *args.begin();
            args.erase(args.begin());
            break;
        case Command::Disconnect:
            if (args.begin() == args.end()) {
                *err_msg << "cypre_ndb: must specify nbd device or image spec";
                return -EINVAL;
            }
            cfg->devpath = *args.begin();
            args.erase(args.begin());
            break;
        default:
            // shut up gcc;
            break;
    }

    if (args.begin() != args.end()) {
        *err_msg << "cypre_ndb: unknown args: " << *args.begin();
        return -EINVAL;
    }

    *command = cmd;
    return 0;
}

int GetMountList(std::map<std::string, std::string> &mount_map) {
    char buf[PATH_MAX] = { 0 };
    memset(buf, 0, sizeof(buf));
    FILE *stream = fopen("/proc/mounts", "r");
    if (stream == NULL) {
        std::cerr << "open /proc/mounts failed: " << CppStrerror(errno) << std::endl;
        return 0;
    }
    while (NULL != fgets(buf, sizeof(buf), stream)) {
        char devname[100];
        char dir[PATH_MAX];
        if(2 != sscanf(buf, "%s %s", devname, dir))
            continue;
        mount_map[devname] = dir;
    }
    fclose(stream);
    return mount_map.size();
}

int GetMappedInfo(int pid, NBDConfig *cfg, bool show_err) {
    int r;
    std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream ifs;
    std::string cmdline;
    std::vector<const char *> args;

    ifs.open(path.c_str(), std::ifstream::in);
    if (!ifs.is_open()) {
        if (show_err) LOG(ERROR) << "GetMappedInfo open failed " << path;
        return -1;
    }
    ifs >> cmdline;

    for (uint i = 0; i < cmdline.size(); i++) {
        char *arg = &cmdline[i];
        if (i == 0) {
            if (strcmp(basename(arg), PROCESS_NAME) != 0) {
                if (show_err)
                    LOG(ERROR)
                            << "GetMappedInfo basename(arg)=" << basename(arg);
                return -EINVAL;
            }
        } else {
            args.push_back(arg);
        }

        while (cmdline[i] != '\0') {
            i++;
        }
    }

    std::ostringstream err_msg;
    Command command;
    r = ParseArgs(args, &err_msg, &command, cfg);
    if (r < 0) {
        if (show_err)
            LOG(ERROR) << "GetMappedInfo ParseArgs failed , cmdline="
                       << cmdline;
        return r;
    }

    if (command != Command::Connect) {
        if (show_err)
            LOG(ERROR) << "GetMappedInfo command != Command::Connect , "
                          "cmdline=%s"
                       << cmdline;
        return -ENOENT;
    }

    return 0;
}

int64_t GetSizeFromFile(const std::string &path) {
    std::ifstream ifs;
    ifs.open(path.c_str(), std::ifstream::in);
    if (!ifs.is_open()) {
        LOG(ERROR) << "cypre_ndb: failed to open " << path << std::endl;
        return -EINVAL;
    }

    uint64_t size = 0;
    ifs >> size;
    return size;
}

int CheckSizeFromFile(const std::string &path, uint64_t expected_size) {
    std::ifstream ifs;
    ifs.open(path.c_str(), std::ifstream::in);
    if (!ifs.is_open()) {
        LOG(ERROR) << "cypre_ndb: failed to open " << path;
        return -EINVAL;
    }

    uint64_t size = 0;
    ifs >> size;
    size *= NBD_BLKSIZE;

    if (size == 0) {
        // Newer kernel versions will report real size only after nbd
        // connect. Assume this is the case and return success.
        return 0;
    }

    if (size != expected_size) {
        LOG(ERROR) << "cypre_ndb: kernel reported invalid size (" << size
                   << ", expected " << expected_size << ")";
        return -EINVAL;
    }

    return 0;
}

int CheckBlockSize(int nbd_index, uint64_t expected_size) {
    std::string path = "/sys/block/nbd" + std::to_string(nbd_index)
                       + "/queue/hw_sector_size";
    int ret = CheckSizeFromFile(path, expected_size);
    if (ret < 0) {
        return ret;
    }
    path = "/sys/block/nbd" + std::to_string(nbd_index)
           + "/queue/minimum_io_size";
    ret = CheckSizeFromFile(path, expected_size);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int64_t GetDeviceSize(int nbd_index) {
    // There are bugs with some older kernel versions that result in an
    // overflow for large image sizes. This check is to ensure we are
    // not affected.
    std::string path = "/sys/block/nbd" + std::to_string(nbd_index) + "/size";
    // Size of the partition in standard UNIX 512-byte sectors
    return GetSizeFromFile(path) * 512;
}

int CheckDeviceSize(int nbd_index, uint64_t expected_size) {
    // There are bugs with some older kernel versions that result in an
    // overflow for large image sizes. This check is to ensure we are
    // not affected.

    std::string path = "/sys/block/nbd" + std::to_string(nbd_index) + "/size";
    return CheckSizeFromFile(path, expected_size);
}

static int run_command(const char *command) {
    int status;

    status = system(command);
    if (status >= 0 && WIFEXITED(status)) return WEXITSTATUS(status);

    if (status < 0) {
        char error_buf[80];
        if (strerror_r(errno, error_buf, sizeof(error_buf)) == nullptr)
            return -1;
        fprintf(stderr, "couldn't run '%s': %s\n", command, error_buf);
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "'%s' killed by signal %d\n", command,
                WTERMSIG(status));
    } else {
        fprintf(stderr, "weird status from '%s': %d\n", command, status);
    }

    return -1;
}

static int module_load(const char *module, const char *options) {
    char command[128];

    snprintf(
            command, sizeof(command), "/sbin/modprobe %s %s", module,
            (options ? options : ""));

    return run_command(command);
}

int LoadModule(NBDConfig *cfg) {
    std::ostringstream param;
    int ret;

    if (cfg->nbds_max) {
        param << "nbds_max=" << cfg->nbds_max;
    }

    if (cfg->max_part) {
        param << " max_part=" << cfg->max_part;
    }

    if (!access("/sys/module/nbd", F_OK)) {
        if (cfg->nbds_max || cfg->set_max_part) {
            LOG(ERROR) << "cypre_ndb: ignoring kernel module parameter options:"
                       << " nbd module already loaded. "
                       << "nbds_max: " << cfg->nbds_max
                       << ", set_max_part: " << cfg->set_max_part;
        }
        return 0;
    }

    ret = module_load("nbd", param.str().c_str());
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: failed to load nbd kernel module: "
                   << CppStrerror(-ret);
    }

    return ret;
}

bool NBDListIterator::Get(DeviceInfo *pinfo) {
    if (show_err_)
        LOG(NOTICE) << "NBDListIterator::Get curIndex=" << cur_index_;

    int nbd_max_count = GetNbdMaxCount();
    for (; cur_index_ < nbd_max_count; cur_index_++) {
        std::string nbd_path = NBD_PATH_PREFIX + std::to_string(cur_index_);
        if (::access(nbd_path.c_str(), F_OK) != 0) {
            if (show_err_) LOG(ERROR) << "access failed " << nbd_path;
            continue;
        }

        DeviceInfo info;
        std::ifstream ifs;
        ifs.open(nbd_path + "/pid", std::ifstream::in);
        if (!ifs.is_open()) {
            if (show_err_) LOG(ERROR) << "fail to open " << nbd_path << "/pid";
            continue;
        }
        ifs >> info.pid;
        info.size = GetDeviceSize(cur_index_);
        if (info.size < 0) continue;
        info.devpath = DEV_PATH_PREFIX + std::to_string(cur_index_);

        NBDConfig config;
        int r = GetMappedInfo(info.pid, &config, show_err_);
        if (r < 0) {
            if (show_err_) LOG(ERROR) << "GetMappedInfo failed " << nbd_path;
            continue;
        }
        info.blob_id = config.imgname;
        std::map<std::string, std::string> mount_map;
        GetMountList(mount_map);
        if (mount_map.find(info.devpath) != mount_map.end())
            info.mnt_point = mount_map[info.devpath];
        cur_index_++;
        *pinfo = info;
        return true;
    }
    return false;
}

ssize_t SafeRead(int fd, void *buf, size_t count) {
    size_t cnt = 0;

    while (cnt < count) {
        ssize_t r = read(fd, buf, count - cnt);
        if (r <= 0) {
            if (r == 0) {  // EOF
                return cnt;
            }
            if (errno == EINTR) {
                continue;
            }
            return -errno;
        }
        cnt += r;
        buf = (char *)buf + r;  // NOLINT
    }
    return cnt;
}

ssize_t SafeReadExact(int fd, void *buf, size_t count) {
    ssize_t ret = SafeRead(fd, buf, count);
    if (ret < 0) {
        return ret;
    }
    if ((size_t)ret != count) {
        return -EDOM;
    }
    return 0;
}

ssize_t SafeWrite(int fd, const void *buf, size_t count) {
    while (count > 0) {
        ssize_t r = write(fd, buf, count);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -errno;
        }
        count -= r;
        buf = (char *)buf + r;  // NOLINT
    }
    return 0;
}

int NbdErrno(int errcode) {
    switch (errcode) {
        case EPERM:
            return htonl(1);
        case EIO:
            return htonl(5);
        case ENOMEM:
            return htonl(12);
        case EINVAL:
            return htonl(22);
        case EFBIG:
        case ENOSPC:
#ifdef EDQUOT
        case EDQUOT:
#endif
            return htonl(28);  // ENOSPC
        default:
            return htonl(22);  // EINVAL
    }
}

}  // namespace nbd
}  // namespace cyprestore
