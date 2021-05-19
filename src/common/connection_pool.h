/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_COMMON_CONNECTION_POOL_H_
#define CYPRESTORE_COMMON_CONNECTION_POOL_H_

#include <brpc/channel.h>

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "rwlock.h"

namespace cyprestore {
namespace common {

struct Connection;
typedef std::shared_ptr<Connection> ConnectionPtr;

struct Connection {
    std::unique_ptr<brpc::Channel> channel;
};

class ConnectionPool {
public:
    ConnectionPool() = default;
    ~ConnectionPool() = default;

    ConnectionPtr GetConnection(const std::string &ip, int port) {
        std::string conn_id = get_connection_id(ip, port);
        {
            ReadLock lock(lock_);
            auto it = connection_pool_.find(conn_id);
            if (it != connection_pool_.end()) return it->second;
        }

        {
            WriteLock lock(lock_);
            auto it = connection_pool_.find(conn_id);
            if (it != connection_pool_.end()) return it->second;

            auto conn = std::make_shared<Connection>();
            conn->channel.reset(new brpc::Channel());
            // TODO: 设置channel options
            if (conn->channel->Init(conn_id.c_str(), nullptr) != 0) {
                return nullptr;
            }
            connection_pool_[conn_id] = conn;
            return conn;
        }
    }

private:
    std::string get_connection_id(const std::string &ip, int port) {
        std::stringstream ss;
        ss << ip << ":" << port;
        return ss.str();
    }

    std::unordered_map<std::string, ConnectionPtr> connection_pool_;
    RWLock lock_;
};

// WARNING(zhangliang): NOT Thread safe
class ConnectionPool2 {
public:
    ConnectionPool2() = default;
    ~ConnectionPool2() = default;

    // TODO(zhangliang): use ip | port ?
    ConnectionPtr GetConnection(int es_id) {
        auto it = connection_pool_.find(es_id);
        if (it != connection_pool_.end()) {
            return it->second;
        }
        return nullptr;
    }

    ConnectionPtr NewConnection(int es_id, const std::string &ip, int port) {
        std::string conn_id = get_connection_id(ip, port);
        auto conn = std::make_shared<Connection>();
        conn->channel.reset(new brpc::Channel());
        // TODO: 设置channel options
        if (conn->channel->Init(conn_id.c_str(), nullptr) != 0) {
            return nullptr;
        }
        connection_pool_[es_id] = conn;
        return conn;
    }

    void Erase(int es_id) {
        connection_pool_.erase(es_id);
    }
    void Clear() {
        connection_pool_.clear();
    }

private:
    std::string get_connection_id(const std::string &ip, int port) {
        std::stringstream ss;
        ss << ip << ":" << port;
        return ss.str();
    }

    std::unordered_map<uint64_t, ConnectionPtr> connection_pool_;
};

}  // namespace common

// common::ConnectionPool &GlobalConnPool() {
//	static common::ConnectionPool global_conn_pool;
//	return global_conn_pool;
//}

}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_CONNECTION_POOL_H_
