/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

/*
 * 支持的对象和命令
 * 1. User
 *    1.1 创建
 *    1.2 删除
 *    1.3 查询
 *    1.4 列表
 *    1.5 重命名
 * 2. Blob
 *    2.1 创建
 *    2.2 删除
 *    2.3 查询
 *    2.4 列表
 *    2.5 重命名
 *    2.6 扩容
 * 3. Router
 *    3.1 分配
 *    3.2 获取
 */

#include <brpc/server.h>
#include <gflags/gflags.h>

#include <iostream>

#include "blob.h"
#include "direct_blob.h"
#include "direct_router.h"
#include "direct_user.h"
#include "options.h"
#include "router_bench.h"
#include "scrub.h"
#include "set.h"
#include "user.h"

/* Brpc */
DEFINE_string(protocal, "baidu_std", "protocal types");
DEFINE_string(
        connection_type, "pooled",
        "Type of connections: single, pooled, short");
DEFINE_int32(timeout_ms, 1000, "RPC timeout in milliseconds");
DEFINE_int32(connection_timeout_ms, 500, " connection timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Maximum retry times by RPC framework");
DEFINE_int32(dummy_port, 8888, "Port of dummy server");

/* User */
DEFINE_string(user_name, "", "user name");
DEFINE_string(user_new_name, "", "user new name");
DEFINE_string(user_id, "", "user id");
DEFINE_string(email, "", "email");
DEFINE_string(new_email, "", "new email");
DEFINE_string(comments, "", "comments");
DEFINE_string(new_comments, "", "new comments");
DEFINE_string(pool_id, "", "pool_id");
DEFINE_string(pool_type, "", "pool_type");
DEFINE_int32(set_id, -1, "set_id");

/* Blob */
DEFINE_string(blob_name, "", "blob name");
DEFINE_string(blob_new_name, "", "blob new name");
DEFINE_string(blob_id, "", "blob id");
DEFINE_string(blob_type, "", "blob type");
DEFINE_string(instance_id, "", "instance id");
DEFINE_string(blob_desc, "", "blob_desc");
DEFINE_uint64(blob_size, 0, "blob size");
DEFINE_uint64(blob_new_size, 0, "blob new size");

/* Extent */
DEFINE_string(extent_id, "", "extent id");
DEFINE_int64(offset, 0, "extent i/o offset");
DEFINE_int64(size, 0, "extent i/o size");
DEFINE_int32(block_size, 4096, "block size");
DEFINE_int32(iodepth, 0, "i/o depth");
DEFINE_int32(jobs, 0, "number of jobs");

/* Router */
DEFINE_int32(extent_idx, 0, "extent idx");

/* Access */
DEFINE_string(access_ip, "", "access ip");
DEFINE_int32(access_port, 0, "access port");

/* ExtentManager */
DEFINE_string(extentmanager_ip, "", "extentmanager ip");
DEFINE_int32(extentmanager_port, 0, "extentmanager port");

/* Object */
DEFINE_string(object, "", "object name");

/* Command */
DEFINE_string(command, "", "command name");

/* bench */
DEFINE_int32(thread_num, 0, "thread num");
DEFINE_int32(blob_num, 0, "blob num");

using namespace cyprestore::tools;

int setup_options(Options &options);
void help();

int setup_options(Options &options) {
    if (FLAGS_object.empty() || FLAGS_command.empty()) {
        std::cerr << "object or command empty." << std::endl;
        return -1;
    }
    options.obj = getObject(FLAGS_object);
    options.cmd = getCmd(FLAGS_command);

    /* User */
    options.user_name = FLAGS_user_name;
    options.user_new_name = FLAGS_user_new_name;
    options.user_id = FLAGS_user_id;
    options.email = FLAGS_email;
    options.new_email = FLAGS_new_email;
    options.comments = FLAGS_comments;
    options.new_comments = FLAGS_new_comments;
    options.pool_id = FLAGS_pool_id;
    options.pool_type = FLAGS_pool_type;
    options.set_id = FLAGS_set_id;

    /* Blob */
    options.blob_name = FLAGS_blob_name;
    options.blob_new_name = FLAGS_blob_new_name;
    options.blob_id = FLAGS_blob_id;
    options.blob_type = FLAGS_blob_type;
    options.instance_id = FLAGS_instance_id;
    options.blob_desc = FLAGS_blob_desc;
    options.blob_size = FLAGS_blob_size;
    options.blob_new_size = FLAGS_blob_new_size;

    /* Extent */
    options.extent_id = FLAGS_extent_id;
    options.offset = static_cast<uint32_t>(FLAGS_offset);
    options.size = static_cast<uint32_t>(FLAGS_size);
    options.iodepth = FLAGS_iodepth;
    options.jobs = FLAGS_jobs;

    /* router */
    options.extent_idx = FLAGS_extent_idx;

    /* Brpc */
    options.protocal = FLAGS_protocal;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms;
    options.connection_timeout_ms = FLAGS_connection_timeout_ms;
    options.max_retry = FLAGS_max_retry;
    options.dummy_port = FLAGS_dummy_port;

    /* Access */
    options.access_ip = FLAGS_access_ip;
    options.access_port = FLAGS_access_port;

    /* ExtentManager */
    options.extentmanager_ip = FLAGS_extentmanager_ip;
    options.extentmanager_port = FLAGS_extentmanager_port;

    /* bench */
    options.thread_num = FLAGS_thread_num;
    options.blob_num = FLAGS_blob_num;

    return 0;
}

void help() {
    std::cout << "\n\t usage:" << std::endl;
}

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    Options options;

    if (setup_options(options) != 0) {
        help();
        return -1;
    }

    int ret = 0;
    switch (options.obj) {
        case Object::kUser: {
            User user(options);
            ret = user.Exec();
            if (ret != 0) {
                std::cerr << "failed to run user" << std::endl;
                return -1;
            }
        } break;
        case Object::kSet: {
            Set set(options);
            ret = set.Exec();
            if (ret != 0) {
                std::cerr << "failed to run set" << std::endl;
                return -1;
            }
        } break;
        case Object::kDUser: {
            DirectUser user(options);
            ret = user.Exec();
            if (ret != 0) {
                std::cerr << "failed to run user" << std::endl;
                return -1;
            }
        } break;
        case Object::kBlob: {
            Blob blob(options);
            ret = blob.Exec();
            if (ret != 0) {
                std::cerr << "failed to run blob" << std::endl;
                return -1;
            }
        } break;
        case Object::kDBlob: {
            DirectBlob blob(options);
            ret = blob.Exec();
            if (ret != 0) {
                std::cerr << "failed to run blob" << std::endl;
                return -1;
            }
        } break;
        case Object::kDRouter: {
            DirectRouter router(options);
            ret = router.Exec();
            if (ret != 0) {
                std::cerr << "failed to run router" << std::endl;
                return -1;
            }
        } break;
        case Object::kRouterBench: {
            RouterBench routerbench(options);
            ret = routerbench.Exec();
            if (ret != 0) {
                std::cerr << "failed to run routerbench" << std::endl;
                return -1;
            }
        } break;
        case Object::kScrub: {
            Scrub scrub(options);
            ret = scrub.Exec();
            if (ret != 0) {
                std::cerr << "faild to run scrub" << std::endl;
            }
            break;
        }
        default:
            break;
    }

    return 0;
}
