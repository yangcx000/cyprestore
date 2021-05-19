/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include <brpc/channel.h>
#include <gflags/gflags.h>

#include <iostream>

#include "extentserver.h"
#include "heartbeat.h"
#include "options.h"
#include "pool.h"
#include "replica_group.h"

/*
 * 支持的命令:
 * 1. Pool
 *   1.1 创建
 *   1.2 查询
 *   1.3 列表
 *   1.4 重命名
 *   1.5 初始化
 *
 * 2. ExtentServer
 *   2.1 查询
 *   2.2 列表
 *
 * 3. Replication Group
 *   3.1 查询
 *   3.2 列表
 *
 * */

DEFINE_string(object, "", "object name");
DEFINE_string(command, "", "command name");

/* Brpc */
DEFINE_string(protocal, "baidu_std", "protocal types");
DEFINE_string(
        connection_type, "", "Type of connections: single, pooled, short");
DEFINE_int32(timeout_ms, 1000, "RPC timeout in milliseconds");
DEFINE_int32(connection_timeout_ms, 500, " connection timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Maximum retry times by RPC framework");

/* Pool */
DEFINE_string(pool_name, "", "pool name");
DEFINE_string(pool_new_name, "", "pool new name");
DEFINE_string(pool_id, "", "pool id");
DEFINE_string(pool_type, "", "pool type");
DEFINE_string(failure_domain, "", "failure_domain");
DEFINE_int32(replica_count, 0, "replica count");
DEFINE_int32(rg_count, 0, "rg count");
DEFINE_int32(es_count, 0, "es count");

/* ExtentServer */
DEFINE_int32(es_id, -1, "es id");
DEFINE_string(es_host, "", "es host");
DEFINE_string(es_rack, "", "es rack");

/* Replication Group */
DEFINE_string(rg_id, "", "rg id");

/* ExtentManager */
DEFINE_string(em_ip, "", "em ip");
DEFINE_int32(em_port, -1, "em port");

using namespace cyprestore::tools;

int set_option(Options &options);
void help();

int set_option(Options &options) {
    if (FLAGS_object.empty() || FLAGS_command.empty()) {
        std::cerr << "object or command empty" << std::endl;
        return -1;
    }

    options.object = FLAGS_object;
    options.cmd = FLAGS_command;
    options.protocal = FLAGS_protocal;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms;
    options.connection_timeout_ms = FLAGS_connection_timeout_ms;
    options.max_retry = FLAGS_max_retry;

    options.pool_name = FLAGS_pool_name;
    options.pool_new_name = FLAGS_pool_new_name;
    options.pool_id = FLAGS_pool_id;
    options.pool_type = FLAGS_pool_type;
    options.failure_domain = FLAGS_failure_domain;
    options.replica_count = FLAGS_replica_count;
    options.rg_count = FLAGS_rg_count;
    options.es_count = FLAGS_es_count;
    options.es_id = FLAGS_es_id;
    options.es_rack = FLAGS_es_rack;
    options.es_host = FLAGS_es_host;
    options.rg_id = FLAGS_rg_id;
    options.em_ip = FLAGS_em_ip;
    options.em_port = FLAGS_em_port;

    return 0;
}

void help() {
    std::cout << "Usage:"
              << "\n\t -object: object name [pool|es|rg]"
              << "\n\t -command: command name [create|query|list|rename|init]"
              << "\n\t -protocal: supported protocal [baidu_std]"
              << "\n\t -connection_type: connection type [single|short|pooled]"
              << "\n\t -timeout_ms: request timeout ms"
              << "\n\t -connection_timeout_ms: connection timeout ms"
              << "\n\t -max_retry: request max retry counts"
              << "\n\t -pool_name: pool name"
              << "\n\t -pool_new_name: pool new name for rename operation"
              << "\n\t -pool_id: pool id"
              << "\n\t -pool_type: pool type [hdd|ssd|nvmessd]"
              << "\n\t -failure_domain: failure domain [node|host|rack]"
              << "\n\t -replica_count: replica count"
              << "\n\t -rg_count: replication group count"
              << "\n\t -es_count: extentserver count"
              << "\n\t -es_id: extentserver id"
              << "\n\t -es_name: extentserver name"
              << "\n\t -es_host: extentserver host"
              << "\n\t -es_rack: extentserver rack"
              << "\n\t -rg_id: replication group id"
              << "\n\t -em_ip: extentmanager ip"
              << "\n\t -em_port: extentmanager port" << std::endl;
}

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    Options options;

    if (set_option(options) != 0) {
        help();
        return -1;
    }

    brpc::Channel channel;
    brpc::ChannelOptions channel_options;
    channel_options.protocol = options.protocal;
    channel_options.connection_type = options.connection_type;
    channel_options.timeout_ms = options.timeout_ms;
    channel_options.max_retry = options.max_retry;

    if (channel.Init(options.Addr().c_str(), &channel_options) != 0) {
        std::cerr << "Failed to init channel, addr:" << options.Addr()
                  << std::endl;
        return -1;
    }

    int ret = 0;
    switch (GetObject(options.object)) {
        case Object::kPool: {
            Pool pool(options, &channel);
            ret = pool.Run();
            if (ret != 0) {
                std::cerr << "Failed to run pool" << std::endl;
                return -1;
            }
        } break;
        case Object::kExtentServer: {
            ExtentServer es(options, &channel);
            ret = es.Run();
            if (ret != 0) {
                std::cerr << "Failed to run extentserver" << std::endl;
                return -1;
            }
        } break;
        case Object::kReplicationGroup: {
            ReplicaGroup rg(options, &channel);
            ret = rg.Run();
            if (ret != 0) {
                std::cerr << "Failed to run replicagroup" << std::endl;
                return -1;
            }
        } break;
        case Object::kHeartbeat: {
            Heartbeat hb(options, &channel);
            ret = hb.Run();
            if (ret != 0) {
                std::cerr << "Failed to run heartbeat" << std::endl;
                return -1;
            }
        } break;
        default:
            std::cerr << "Unknown object, object:" << options.object
                      << std::endl;
            break;
    }

    return 0;
}
