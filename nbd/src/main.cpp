/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#include <brpc/server.h>
#include <butil/logging.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

#include <iostream>
#include <string>
#include <list>

#include "argparse.h"
#include "blob_instance.h"
#include "common/config.h"
#include "common/log.h"
#include "config.h"
#include "dump.h"
#include "mylogsink.h"
#include "nbd_ctrl.h"
#include "nbd_mgr.h"
#include "nbd_server.h"
#include "nbd_watch.h"
#include "plus.h"
#include "texttable.h"
#include "util.h"

namespace cyprestore {
namespace nbd {

// environment passed by parent to child process
#define EXEC_CHILD_PIPE_FD "__CYPRE_NBD_EXEC_CHILD_PIPE_FD__"

#if ! defined(GIT_COMMIT_ID)
#define GIT_COMMIT_ID "000000" 
#endif
const std::string kVersion = "1.0 (" GIT_COMMIT_ID ")";
std::shared_ptr<NBDMgr> g_nbd_mgr;
std::shared_ptr<NBDConfig> g_nbd_config;
MyLogSink g_logSink;

static void HandleSignal(int signum) {
    LOG(NOTICE) << "HandleSignal " << sys_siglist[signum]
              << ", pid : " << getpid();

    if (signum == SIGSEGV || signum == SIGBUS || signum == SIGFPE
        || signum == SIGILL) {
        g_logSink.Flush(logging::BLOG_ERROR);
        g_logSink.Flush(logging::BLOG_INFO);
        DumpErrorStack(signum);
        ::raise(signum);
        // should not get here, process maybe can't exit normally, so kill -9
        ::kill(::getpid(), SIGKILL);
    } else if (signum != SIGINT && signum != SIGTERM) {
        LOG(NOTICE) << "Catch unexpected signal : " << signum;
        return;
    }
    if (IsMounted(g_nbd_config->devpath)) {
        LOG(NOTICE) << "ignore signal: " << signum << " when mounted";
        return;
    }
    int ret = g_nbd_mgr->Disconnect(g_nbd_config->devpath);
    if (ret != 0) {
        LOG(ERROR) << g_nbd_config->devpath
                   << "  disconnect failed. Error: " << ret;
    } else {
        LOG(NOTICE) << g_nbd_config->devpath << "  disconnected end";
    }
}

static void Usage() {
    std::cout << "Usage: cypre_ndb <command> ...\n"
              << "\n"
              << "<command>:\n"
              << "  map <blobid> [options]  map blob to nbd device\n"
              << "  unmap <device>          unmap nbd device\n"
              << "  list                    list mapped nbd devices\n"
              << "\n"
              << "map options:\n"
              << "  -device=<device path>  nbd device path(/dev/nbd{num})\n"
              << "  -readonly              map readonly\n"
              << "  -nbds_max=<limit>      override for module param nbds_max\n"
              << "  -max_part=<limit>      override for module param max_part\n"
              << "  -timeout=<seconds>     set nbd request timeout\n"
              /* TODO: << "  -try_netlink           use the nbd netlink interface\n" */
              << "  -multi_conns=<conns>   multiple connections to a single nbd device, default is 1\n"
              << "  -em_endpoint           extentmanager server ip and port(ip:port)\n"
              << "  -client_core_mask=<brpc_core>\n"
              << "                         set brpc worker core mask\n"
              << "  -debug                 show  debug  message \n"
              << "  -f                     run in foreground \n"
              << "  -nullio                run empty io for test\n"
              << "  -dummy_port=<port>     brpc dummy port\n"
              << "common options:\n"
              << "  -h -help               show help\n"
              << "  -v -version            show version\n"
              << std::endl;
}
/*
static bool InitLog(const std::string &logpath, const std::string &prefix) {
    common::LogCfg cfg;
    cfg.maxsize = 1048576 * 500;
    cfg.level = g_nbd_config->debug ? "INFO" : "NOTICE";
    cfg.logpath = logpath;
    cfg.prefix = prefix;
    cfg.suffix = ".log";
    common::Log log(cfg);
    if (log.Init() != 0) {
        return false;
    }

    return true;
}
*/

static int NBDConnect(int argc, const char *argv[], char *const envp[]) {
    // init log
    std::string prefix;
    std::string devpath = g_nbd_config->devpath;
    if (devpath.length() > 0) {
        int p = devpath.length() - 1;
        for (; p >= 0; p--) {
            if (devpath.at(p) == '/') break;
        }
        if (p > 0) prefix = devpath.substr(p + 1);
    }
    std::string logpath = g_nbd_config->logpath;
    if (logpath.empty()) logpath = "/var/log/cyprestore/cypre_nbd/";
    ForceDirStrong(logpath.c_str());

    int nbd_index = cyprestore::nbd::ParseNbdIndex(g_nbd_config->devpath);

    // if (!InitLog(logpath, prefix)) return -1;

    bool foreground = g_nbd_config->foreground;
    int wait_connect_pipe[2] = {-1, -1};
    int connect_result = -1;
    if (foreground) {
        g_logSink.Init(logpath, std::to_string(nbd_index), 100);
        ::logging::SetLogSink(&g_logSink);
        // check whether if fork() from parent
        std::string exec_env = std::string(EXEC_CHILD_PIPE_FD) + "=";
        for(auto t = envp; *t != NULL; ++t) {
            std::string env = *t;
            if (env.compare(0, exec_env.size(), exec_env) == 0 && env != exec_env) {
                wait_connect_pipe[1] = std::stoi(env.substr(exec_env.size()));
                LOG(NOTICE) << "find pipe fd=" << wait_connect_pipe[1];
                foreground = false;
                break;
            }
        }
        if (!foreground) {
            if (::chdir("/") < 0) {
                LOG(ERROR) << "fail to chdir /: " << CppStrerror(errno);
            }
        }

    } else {
        if (0 != pipe(wait_connect_pipe)) {
            std::cerr << "create pipe failed" << std::endl;
            return -1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "fork failed, " << CppStrerror(errno) << std::endl;
            return -1;
        }

        g_logSink.Init(logpath, std::to_string(nbd_index), 100);
        ::logging::SetLogSink(&g_logSink);

        // parent  progress
        if (pid > 0) {
            int nr = read(wait_connect_pipe[0], &connect_result, sizeof(connect_result));
            if (nr != sizeof(connect_result)) {
                LOG(ERROR) << "Read from child failed, " << CppStrerror(errno)
                           << ", nr = " << nr << std::endl;
            }

            if (connect_result < 0) {
                // wait child process exit
                std::cerr << "map failed: " << connect_result << " " << CppStrerror(-connect_result) << std::endl;
            } else {
                std::cout << "map success: /dev/nbd" << connect_result << std::endl;
            }

            return connect_result >= 0 ? 0 : -1;
        }

        // in child
        setsid();
        umask(0);

        // close  STDOUT
        int fd = -1;
        if ((fd = ::open("/dev/null", O_RDWR)) >= 0) {
            ::dup2(fd, STDIN_FILENO);
            ::dup2(fd, STDOUT_FILENO);
            ::dup2(fd, STDERR_FILENO);
            if (fd > STDERR_FILENO) ::close(fd);
        } else {
            LOG(ERROR) << "fail to open /dev/null: " << CppStrerror(errno);
        }
        // close unusable fd
        std::list<int> fds;
        auto dir = ::opendir("/proc/self/fd");
        if (dir != NULL) {
            struct dirent* ent;
            while (NULL != (ent=::readdir(dir))) {
                if(0 == strcmp(".", ent->d_name) || 0 == strcmp("..", ent->d_name))
                    continue;
                int fd = std::stoi(ent->d_name);
                if(fd > STDERR_FILENO && fd != wait_connect_pipe[1])
                    fds.push_back(fd);
            }
            ::closedir(dir);
            for(auto fd: fds)
                ::close(fd);
        }
        // try to exec self to avoid middle-status of brpc
        // brpc already started sample thread, it maybe make global locks in middle-status
        char** newargv = new char*[argc + 2];
        memcpy(newargv, argv, argc * sizeof(char*));
        // add foreground argument
        newargv[argc] = strdup("-f");
        newargv[argc + 1] = NULL;
        // pass wait_connect_pipe[1] as EXEC_CHILD_PIPE_FD environment variable to child
        int envn = 0;
        for(; envp[envn] != NULL; ++envn);
        char** newenv = new char*[envn + 2];
        memcpy(newenv, envp, envn * sizeof(char*));
        auto exec_env = std::string(EXEC_CHILD_PIPE_FD) + "=" + std::to_string(wait_connect_pipe[1]);
        newenv[envn] = strdup(exec_env.c_str());
        newenv[envn + 1] = NULL;
        if(::execve(argv[0], newargv, newenv) < 0) {
            LOG(ERROR) << "fail to execve: " << CppStrerror(errno);
            int ret = -errno;
            if (::write(wait_connect_pipe[1], &ret, sizeof(ret)) < 0)
                std::cerr << "write(wait_connect_pipe[1]) failed: " << CppStrerror(errno) << std::endl;
            return ret;
        }
    }

    // set signal handler
    signal(SIGTERM, HandleSignal);
    signal(SIGINT, HandleSignal);
    // ignore these signals to keep process from killed wrongly
    // signal explaination: https://www.21ic.com/tougao/article/12651.html
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGXCPU, SIG_IGN);
    signal(SIGXFSZ, SIG_IGN);
    signal(SIGVTALRM, SIG_IGN);
    signal(SIGPROF, SIG_IGN);

    signal(SIGSEGV, HandleSignal);
    signal(SIGBUS, HandleSignal);
    signal(SIGFPE, HandleSignal);
    signal(SIGILL, HandleSignal);

    if (g_nbd_config->dummy_port != 0) {
        brpc::StartDummyServerAt(g_nbd_config->dummy_port);
    }

    int ret = g_nbd_mgr->Connect(g_nbd_config.get());
    if (ret < 0) {
        if (!foreground) {
            if (::write(wait_connect_pipe[1], &ret, sizeof(ret)) < 0)
                std::cerr << "write(wait_connect_pipe[1]) failed: " << CppStrerror(errno) << std::endl;
        } else
            std::cerr << "Connect failed: " << ret << " " << CppStrerror(-ret) << std::endl;

        LOG(NOTICE) << "Connect failed: " << CppStrerror(-ret);
        return ret;
    } else {
        connect_result = 0;
        if (!foreground) {
            int index = cyprestore::nbd::ParseNbdIndex(g_nbd_config->devpath);
            assert(index >= 0);
            // send nbd index to parent
            if (::write(wait_connect_pipe[1], &index, sizeof(index)) < 0)
                std::cerr << "write(wait_connect_pipe[1]) failed: " << CppStrerror(errno) << std::endl;
            ::close(wait_connect_pipe[1]);
        } else
            std::cout << "map success: " << g_nbd_config->devpath << std::endl;
        // modify io scheduler to none
        char sys_nbd[100];
        snprintf(sys_nbd, sizeof(sys_nbd), "/sys/block/%s/queue/scheduler", 
                    g_nbd_config->devpath.c_str() + sizeof("/dev/") - 1);
        int fd = ::open(sys_nbd, O_WRONLY);
        if (fd < 0) {
            LOG(ERROR) << "fail to modify io scheduler: " << strerror(errno);
        } else {
            int ret = ::write(fd, "none", sizeof("none") - 1);
            if (ret < 0) {
                LOG(ERROR) << "fail to write io scheduler: " << strerror(errno);
            } else {
                LOG(NOTICE) << "modify io scheduler " << g_nbd_config->devpath << " to none";
            }
            ::close(fd);
        }
        std::cout << "start to RunServerUntilQuit... " << std::endl;
        g_nbd_mgr->RunServerUntilQuit();
        std::cout << "RunServerUntilQuit end " << std::endl;
    }

    return 0;
}

static int NbdMain(int argc, const char *argv[], char *const envp[]) {
    if (argc < 2) {
        Usage();
        exit(0);
    }
    int r = 0;
    Command command;
    std::ostringstream err_msg;
    std::vector<const char *> args;

    g_nbd_config = std::make_shared<NBDConfig>();

    argv_to_vec(argc, argv, args);
    r = ParseArgs(args, &err_msg, &command, g_nbd_config.get());

    if (r == HELP_INFO) {
        Usage();
        return 0;
    } else if (r == VERSION_INFO) {
        std::cout << "cypre_ndb version : " << kVersion << std::endl;
        return 0;
    } else if (r < 0) {
        std::cerr << err_msg.str() << std::endl;
        return r;
    }
    std::string devpath = g_nbd_config->devpath;
    if (devpath.length() > 0) {
        int r = ParseNbdIndex(devpath);
        if (r < 0) {
            return -EINVAL;
        }
    }

    g_nbd_mgr = std::make_shared<NBDMgr>();
    switch (command) {
        case Command::Connect: {
            if (g_nbd_config->imgname.empty()) {
                std::cerr << "Error: image name was not specified" << std::endl;
                return -EINVAL;
            }
            r = NBDConnect(argc, argv, envp);
            if (r < 0) {
                return r;
            }

            break;
        }
        case Command::Disconnect: {
            if (IsMounted(g_nbd_config->devpath)) {
                std::cerr << g_nbd_config->devpath << " already mounted"
                          << ", please umount it first" << std::endl;
                return -EINVAL;
            }

            r = g_nbd_mgr->Disconnect(g_nbd_config->devpath);
            return r;
        }
        case Command::List: {
            std::vector<DeviceInfo> devices;
            g_nbd_mgr->List(&devices, g_nbd_config->debug);
            std::cout << " nbd list count= " << devices.size() << std::endl;
            if (devices.empty()) {
                break;
            }
            TextTable tbl;
            tbl.DefineColumn("device", TextTable::LEFT, TextTable::LEFT);
            tbl.DefineColumn("blobid", TextTable::LEFT, TextTable::LEFT);
            tbl.DefineColumn("size", TextTable::LEFT, TextTable::RIGHT);
            tbl.DefineColumn("pid", TextTable::LEFT, TextTable::LEFT);
            tbl.DefineColumn("mntpoint", TextTable::LEFT, TextTable::LEFT);

            for (const auto &devinfo : devices) {
                tbl << devinfo.devpath << devinfo.blob_id
                    << devinfo.size << devinfo.pid << devinfo.mnt_point << TextTable::endrow;
            }
            std::cout << tbl << std::endl;
            break;
        }
        case Command::None:
        default: {
            Usage();
            break;
        }
    }

    g_nbd_mgr.reset();
    g_nbd_config.reset();

    return 0;
}

}  // namespace nbd
}  // namespace cyprestore

int main(int argc, const char *argv[], char *const envp[]) {
    int r = cyprestore::nbd::NbdMain(argc, argv, envp);
    // g_logSink.Shutdown();
    if (r < 0) {
        // std::cerr << "main " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    //DEBUG("main end pid : %d\n", getpid());

    return 0;
}
