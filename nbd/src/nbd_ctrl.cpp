/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#include "nbd_ctrl.h"

#include <assert.h>

namespace cyprestore {
namespace nbd {

int IOController::InitDevAttr(
        int devfd, NBDConfig *config, const std::vector<int> &socks,
        uint64_t size, uint64_t flags) {
    int ret = 0;
    for (unsigned int i = 0; i < socks.size(); i++) {
        ret = ioctl(devfd, NBD_SET_SOCK, socks.at(i));
        if (ret < 0) {
            LOG(ERROR) << "InitDevAttr: the device " << config->devpath + " "
                       << errno << " " << strerror(errno);
            return -errno;
        } else {
            LOG(INFO) << "InitDevAttr NBD_SET_SOCK success "
                       << config->devpath + " sock=" << socks.at(i);

            std::cout << "InitDevAttr NBD_SET_SOCK success "
                      << config->devpath + " sock=" << socks.at(i) << std::endl;
        }
    }

    do {
        ret = ioctl(devfd, NBD_SET_BLKSIZE, NBD_BLKSIZE);
        if (ret < 0) {
            break;
        }

        /*
        ret = ioctl(devfd, NBD_SET_SIZE, size);
        if (ret < 0)
                break;
        */

        unsigned long blocks = (unsigned long)(size / NBD_BLKSIZE);
        ret = ioctl(devfd, NBD_SET_SIZE_BLOCKS, blocks);
        if (ret < 0) break;

        LOG(INFO) << "InitDevAttr BLKSIZE=" << NBD_BLKSIZE << ", SIZE=" << size
                   << ", blocks=" << blocks;

        std::cout << "InitDevAttr BLKSIZE=" << NBD_BLKSIZE << ", SIZE=" << size
                  << ", blocks=" << blocks << std::endl;

        ioctl(devfd, NBD_SET_FLAGS, flags);

        ret = CheckSetReadOnly(devfd, flags);
        if (ret < 0) {
            LOG(ERROR) << "cypre_ndb: Check and set read only flag failed."
                       << CppStrerror(-ret);
            break;
        }

        if (config->timeout >= 0) {
            ret = ioctl(devfd, NBD_SET_TIMEOUT, (unsigned long)config->timeout);
            if (ret < 0) {
                LOG(ERROR) << "cypre_ndb: failed to set timeout: "
                           << CppStrerror(ret);
                break;
            } else {
                LOG(INFO) << "cypre_ndb: set timeout success: timeout="
                           << config->timeout;

                std::cout << "cypre_ndb: set timeout success: timeout="
                          << config->timeout << std::endl;
            }
        }
    } while (false);

    if (ret < 0) {
        ret = -errno;
        ioctl(devfd, NBD_CLEAR_SOCK);
    }
    return ret;
}

int IOController::SetUp(
        NBDConfig *config, const std::vector<int> &socks, uint64_t size,
        uint64_t flags) {
    if (config->devpath.empty()) {
        config->devpath = FindUnusedNbdDevice();
    }

    if (config->devpath.empty()) {
        return CYPRE_ER_NONE_FREE_NBD;
    }

    int ret = ParseNbdIndex(config->devpath);
    if (ret < 0) {
        return ret;
    }
    int index = ret;

    ret = open(config->devpath.c_str(), O_RDWR);
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: failed to open device: "
                   << config->devpath + " " << errno << " " << strerror(errno);
        return ret;
    }
    int devfd = ret;

    ret = InitDevAttr(devfd, config, socks, size, flags);
    /*
    if (ret == 0)
    {
            ret = CheckDeviceSize(index, size);
    }
    */
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: failed to map, status: " << CppStrerror(ret);
        close(devfd);
        return ret;
    }

    nbd_fd_ = devfd;
    nbd_index_ = index;
    return 0;
}

int IOController::DisconnectByPath(const std::string &devpath) {
    int index = ParseNbdIndex(devpath);
    assert(index >= 0);
    char sys_path[100];

    // devpath exists == /sys/block/nbdX exists
    snprintf(sys_path, sizeof(sys_path), "/sys/block/nbd%d", index);
    if (::access(sys_path, F_OK) < 0) {
        if (errno == ENOENT)
            std::cerr << "Error: " << devpath << " not exists" << std::endl;
        else
            std::cerr << "Error: " << CppStrerror(errno) << std::endl;
        return -errno;
    }
    // check pid file, if not mapped, pid file not exits
    snprintf(sys_path, sizeof(sys_path), "/sys/block/nbd%d/pid", index);
    if (::access(sys_path, F_OK) < 0) {
        if (errno == ENOENT) {
            std::cerr << "Error: " << devpath << " not mapped" << std::endl;
            return 0;
        } else
            std::cerr << "Error: " << CppStrerror(errno) << std::endl;
        return -errno;
    }
    /*
    */
    int devfd = open(devpath.c_str(), O_RDWR);
    if (devfd < 0) {
        std::cerr << "Error: failed to open device: " << devpath
                  << ", error = " << CppStrerror(errno) << std::endl;
        return -errno;
    }

    int ret = ioctl(devfd, NBD_DISCONNECT);
    if (ret < 0) {
        std::cerr << "DisconnectByPath: ioctl(" << devfd
                  << ", NBD_DISCONNECT) error: " << CppStrerror(errno)
                  << std::endl;
        ret = -errno;
    }
    close(devfd);
    std::cout << "unmap success: " << devpath << std::endl;
    return ret;
}

int IOController::Resize(uint64_t size) {
    if (nbd_fd_ < 0) {
        LOG(ERROR) << "resize failed: nbd controller is not setup.";
        return -1;
    }
    int ret = ioctl(nbd_fd_, NBD_SET_SIZE, size);
    if (ret < 0) {
        LOG(ERROR) << "resize failed: " << CppStrerror(errno);
    }
    return ret;
}

int NetLinkController::Init() {
    if (sock_ != nullptr) {
        Uninit();
    }

    struct nl_sock *sock = nl_socket_alloc();
    if (sock == nullptr) {
        LOG(ERROR) << "cypre_ndb: Could not alloc netlink socket. Error "
                   << CppStrerror(errno);
        return -1;
    }

    int ret = genl_connect(sock);
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: Could not connect netlink socket. Error "
                   << nl_geterror(ret);
        nl_socket_free(sock);
        return -1;
    }

    nl_id_ = genl_ctrl_resolve(sock, "nbd");
    if (nl_id_ < 0) {
        LOG(ERROR) << "cypre_ndb: Could not resolve netlink socket. Error "
                   << nl_geterror(nl_id_);
        nl_close(sock);
        nl_socket_free(sock);
        return -1;
    }
    sock_ = sock;
    return 0;
}

void NetLinkController::Uninit() {
    if (sock_ == nullptr) return;

    nl_close(sock_);
    nl_socket_free(sock_);
    sock_ = nullptr;
    nl_id_ = -1;
}

int NetLinkController::SetUp(
        NBDConfig *config, const std::vector<int> &socks, uint64_t size,
        uint64_t flags) {
    int ret = Init();
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: Netlink interface not supported."
                   << " Using ioctl interface.";
        return ret;
    }

    for (unsigned int i = 0; i < socks.size(); i++) {
        ret = ConnectInternal(config, socks.at(i), size, flags);
    }

    Uninit();
    if (ret < 0) {
        return ret;
    }

    int index = ParseNbdIndex(config->devpath);
    if (index < 0) {
        return index;
    }
    ret = CheckBlockSize(index, NBD_BLKSIZE);
    if (ret < 0) {
        return ret;
    }
    ret = CheckDeviceSize(index, size);
    if (ret < 0) {
        return ret;
    }

    int fd = open(config->devpath.c_str(), O_RDWR);
    if (fd < 0) {
        LOG(ERROR) << "cypre_ndb: failed to open device: " << config->devpath;
        return fd;
    }

    ret = CheckSetReadOnly(fd, flags);
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: Check and set read only flag failed.";
        close(fd);
        return ret;
    }

    nbd_fd_ = fd;
    nbd_index_ = index;
    return 0;
}

int NetLinkController::DisconnectByPath(const std::string &devpath) {
    int index = ParseNbdIndex(devpath);
    if (index < 0) {
        return index;
    }

    int ret = Init();
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: Netlink interface not supported."
                   << " Using ioctl interface.";
        return ret;
    }

    ret = DisconnectInternal(index);
    Uninit();
    return ret;
}

int NetLinkController::Resize(uint64_t size) {
    if (nbd_index_ < 0) {
        LOG(ERROR) << "resize failed: nbd controller is not setup.";
        return -1;
    }

    int ret = Init();
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: Netlink interface not supported."
                   << " Using ioctl interface.";
        return ret;
    }

    ret = ResizeInternal(nbd_index_, size);
    Uninit();
    return ret;
}

bool NetLinkController::Support() {
    int ret = Init();
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: Netlink interface not supported."
                   << " Using ioctl interface.";
        return false;
    }
    Uninit();
    return true;
}

static int netlink_connect_cb(struct nl_msg *msg, void *arg) {
    struct genlmsghdr *gnlh = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    NBDConfig *cfg = reinterpret_cast<NBDConfig *>(arg);
    struct nlattr *msg_attr[NBD_ATTR_MAX + 1];
    int ret;

    ret = nla_parse(
            msg_attr, NBD_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);
    if (ret) {
        LOG(ERROR) << "cypre_ndb: Unsupported netlink reply";
        return -NLE_MSGTYPE_NOSUPPORT;
    }

    if (!msg_attr[NBD_ATTR_INDEX]) {
        LOG(ERROR) << "cypre_ndb: netlink connect reply missing device index.";
        return -NLE_MSGTYPE_NOSUPPORT;
    }

    uint32_t index = nla_get_u32(msg_attr[NBD_ATTR_INDEX]);
    cfg->devpath = "/dev/nbd" + std::to_string(index);

    return NL_OK;
}

int NetLinkController::ConnectInternal(
        NBDConfig *config, int sockfd, uint64_t size, uint64_t flags) {
    struct nlattr *sock_attr = nullptr;
    struct nlattr *sock_opt = nullptr;
    struct nl_msg *msg = nullptr;
    int ret;

    nl_socket_modify_cb(
            sock_, NL_CB_VALID, NL_CB_CUSTOM, netlink_connect_cb, config);
    msg = nlmsg_alloc();
    if (msg == nullptr) {
        LOG(ERROR) << "cypre_ndb: Could not allocate netlink message.";
        return -ENOMEM;
    }

    auto user_hdr = genlmsg_put(
            msg, NL_AUTO_PORT, NL_AUTO_SEQ, nl_id_, 0, 0, NBD_CMD_CONNECT, 0);
    if (user_hdr == nullptr) {
        LOG(ERROR) << "cypre_ndb: Could not setup message.";
        goto nla_put_failure;
    }

    if (!config->devpath.empty()) {
        int index = ParseNbdIndex(config->devpath);
        if (index < 0) {
            goto nla_put_failure;
        }
        NLA_PUT_U32(msg, NBD_ATTR_INDEX, index);
    }
    if (config->timeout >= 0) {
        NLA_PUT_U64(msg, NBD_ATTR_TIMEOUT, config->timeout);
    }
    NLA_PUT_U64(msg, NBD_ATTR_SIZE_BYTES, size);
    NLA_PUT_U64(msg, NBD_ATTR_BLOCK_SIZE_BYTES, NBD_BLKSIZE);
    NLA_PUT_U64(msg, NBD_ATTR_SERVER_FLAGS, flags);

    sock_attr = nla_nest_start(msg, NBD_ATTR_SOCKETS);
    if (sock_attr == nullptr) {
        LOG(ERROR) << "cypre_ndb: Could not init sockets in netlink message.";
        goto nla_put_failure;
    }

    sock_opt = nla_nest_start(msg, NBD_SOCK_ITEM);
    if (sock_opt == nullptr) {
        LOG(ERROR) << "cypre_ndb: Could not init sock in netlink message.";
        goto nla_put_failure;
    }

    NLA_PUT_U32(msg, NBD_SOCK_FD, sockfd);
    nla_nest_end(msg, sock_opt);
    nla_nest_end(msg, sock_attr);

    ret = nl_send_sync(sock_, msg);
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: netlink connect failed: " << nl_geterror(ret);
        return -EIO;
    }
    return 0;

nla_put_failure:
    nlmsg_free(msg);
    return -EIO;
}

int NetLinkController::DisconnectInternal(int index) {
    struct nl_msg *msg = nullptr;
    int ret;

    nl_socket_modify_cb(
            sock_, NL_CB_VALID, NL_CB_CUSTOM, genl_handle_msg, NULL);
    msg = nlmsg_alloc();
    if (msg == nullptr) {
        LOG(ERROR) << "cypre_ndb: Could not allocate netlink message.";
        return -EIO;
    }

    auto user_hdr = genlmsg_put(
            msg, NL_AUTO_PORT, NL_AUTO_SEQ, nl_id_, 0, 0, NBD_CMD_DISCONNECT, 0);
    if (user_hdr == nullptr) {
        LOG(ERROR) << "cypre_ndb: Could not setup message.";
        goto nla_put_failure;
    }

    NLA_PUT_U32(msg, NBD_ATTR_INDEX, index);

    ret = nl_send_sync(sock_, msg);
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: netlink disconnect failed: "
                   << nl_geterror(ret);
        return -EIO;
    }

    return 0;

nla_put_failure:
    nlmsg_free(msg);
    return -EIO;
}

int NetLinkController::ResizeInternal(int nbdIndex, uint64_t size) {
    struct nl_msg *msg = nullptr;
    int ret;

    nl_socket_modify_cb(
            sock_, NL_CB_VALID, NL_CB_CUSTOM, genl_handle_msg, NULL);
    msg = nlmsg_alloc();
    if (msg == nullptr) {
        LOG(ERROR) << "error: Could not allocate netlink message.";
        return -EIO;
    }

    auto user_hdr = genlmsg_put(
            msg, NL_AUTO_PORT, NL_AUTO_SEQ, nl_id_, 0, 0, NBD_CMD_RECONFIGURE,
            0);
    if (user_hdr == nullptr) {
        LOG(ERROR) << "cypre_ndb: Could not setup message.";
        goto nla_put_failure;
    }

    NLA_PUT_U32(msg, NBD_ATTR_INDEX, nbdIndex);
    NLA_PUT_U64(msg, NBD_ATTR_SIZE_BYTES, size);

    ret = nl_send_sync(sock_, msg);
    if (ret < 0) {
        LOG(ERROR) << "cypre_ndb: netlink resize failed: " << nl_geterror(ret);
        return -EIO;
    }

    return 0;

nla_put_failure:
    nlmsg_free(msg);
    return -EIO;
}

}  // namespace nbd
}  // namespace cyprestore
