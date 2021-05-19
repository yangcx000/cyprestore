/*
 * Copyright 2020 JDD authors.
 * @zhangliang
 *
 */

#ifndef CYPRESTORE_CLIENTS_LIBCYPRE_COMMON_H_
#define CYPRESTORE_CLIENTS_LIBCYPRE_COMMON_H_

#include <time.h>

namespace cyprestore {
namespace clients {

enum ExtentIoProtocol {
    kBrpc,  // brpc
    kNull   // MUST BE the last one, for null io test only
};

}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_LIBCYPRE_COMMON_H_
