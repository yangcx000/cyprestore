/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_COMMON_ERROR_CODE_H_
#define CYPRESTORE_COMMON_ERROR_CODE_H_

namespace cyprestore {
namespace common {

/* all error code must be negative number,
 * -1 ~ -999 reserved
 *  -1000 ~ -1999 represent common error, like InvalidParameter, prefix:
 * CYPRE_ER_ -2000 ~ -2999 represent client sdk (libcypre) error, prefix:
 * CYPRE_C_ -3000 ~ -3999 represent ExtentManager error, prefix: CYPRE_EM_ -4000
 * ~ -4999 represent ExtentServer error, prefix: CYPRE_ES_ -5000 ~ -5999
 * represent SetManager error, prefix: CYPRE_SM_ -6000 ~ -6999 represent Access
 * error, prefix: CYPRE_AC_
 */

const int CYPRE_OK = 0;

// -1 ~ -999 reserved

// -1000 ~ -1999 common
const int CYPRE_ER_INVALID_ARGUMENT = -1000;
const int CYPRE_ER_NO_PERMISSION = -1001;
const int CYPRE_ER_NOT_SUPPORTED = -1002;
const int CYPRE_ER_TIMEDOUT = -1003;
const int CYPRE_ER_OUT_OF_MEMORY = -1004;
const int CYPRE_ER_NET_ERROR = -1005;

// -2000 ~ -2999 client sdk
const int CYPRE_C_STREAM_CORRUPT = -2000;
const int CYPRE_C_DATA_TOO_LARGE = -2001;
const int CYPRE_C_DEVICE_CLOSED = -2002;
const int CYPRE_C_INTERNAL_ERROR = -2003;
const int CYPRE_C_HANDLE_NOT_FOUND = -2004;
const int CYPRE_C_RING_FULL = -2005;
const int CYPRE_C_RING_EMPTY = -2006;
const int CYPRE_C_CRC_ERROR = -2007;

// -3000 ~ -3999 ExtentManager
const int CYPRE_EM_POOL_NOT_FOUND = -3000;
const int CYPRE_EM_BLOB_NOT_FOUND = -3001;
const int CYPRE_EM_USER_NOT_FOUND = -3002;
const int CYPRE_EM_ROUTER_NOT_FOUND = -3003;
const int CYPRE_EM_ES_NOT_FOUND = -3004;
const int CYPRE_EM_STORE_ERROR = -3005;
const int CYPRE_EM_LOAD_ERROR = -3006;
const int CYPRE_EM_DELETE_ERROR = -3007;
const int CYPRE_EM_DECODE_ERROR = -3008;
const int CYPRE_EM_ENCODE_ERROR = -3009;
const int CYPRE_EM_ES_DUPLICATED = -3010;
const int CYPRE_EM_POOL_ALREADY_INIT = -3011;
const int CYPRE_EM_POOL_NOT_READY = -3012;
const int CYPRE_EM_POOL_DUPLICATED = -3013;
const int CYPRE_EM_POOL_ALLOCATE_FAIL = -3014;
const int CYPRE_EM_RG_ALLOCATE_FAIL = -3015;
const int CYPRE_EM_RG_NOT_FOUND = -3016;
const int CYPRE_EM_USER_DUPLICATED = -3017;
const int CYPRE_EM_EXCEED_TIME_LIMIT = -3018;
const int CYPRE_EM_TRY_AGAIN = -3019;
const int CYPRE_EM_GET_CONN_ERROR = -3020;
const int CYPRE_EM_BRPC_SEND_FAIL = -3021;

// -4000 ~ -4999 ExtentServer
const int CYPRE_ES_EXTENT_EMPTY = -4000;
const int CYPRE_ES_BDEV_NULL = -4001;
const int CYPRE_ES_DISK_EMPTY = -4002;
const int CYPRE_ES_DISK_NO_SPACE = -4003;
const int CYPRE_ES_DISK_NOT_FOUND = -4004;
const int CYPRE_ES_GET_REQ_UNIT_FAIL = -4005;
const int CYPRE_ES_PROCESS_REQ_ERROR = -4006;
const int CYPRE_ES_OPEN_ROCKSDB_ERROR = -4007;
const int CYPRE_ES_CLOSE_ROCKSDB_ERROR = -4008;
const int CYPRE_ES_INIT_SPACE_ALLOC_FAIL = -4009;
const int CYPRE_ES_LOCATION_NOT_FOUND = -4010;
const int CYPRE_ES_ROCKSDB_STORE_ERROR = -4011;
const int CYPRE_ES_ROCKSDB_LOAD_ERROR = -4012;
const int CYPRE_ES_DECODE_ERROR = -4013;
const int CYPRE_ES_REPLICATE_PART_FAIL = -4014;
const int CYPRE_ES_READ_CONFIG_ERROR = -4015;
const int CYPRE_ES_SPDK_INIT_ERROR = -4016;
const int CYPRE_ES_SPDK_BDEV_INIT_ERROR = -4017;
const int CYPRE_ES_SPDK_BDEV_OPEN_ERROR = -4018;
const int CYPRE_ES_SPDK_RPC_CLOSED = -4019;
const int CYPRE_ES_SPDK_RPC_LISTEN_ERROR = -4020;
const int CYPRE_ES_SPDK_RPC_REGISTER_ERROR = -4021;
const int CYPRE_ES_SPDK_THREAD_CREATE_FAIL = -4022;
const int CYPRE_ES_SPDK_THREAD_NULL = -4023;
const int CYPRE_ES_PTHREAD_CREATE_ERROR = -4024;
const int CYPRE_ES_PTHREAD_JOIN_ERROR = -4025;
const int CYPRE_ES_QUERY_ROUTER_FAIL = -4026;
const int CYPRE_ES_RTE_RING_CREATE_FAIL = -4027;
const int CYPRE_ES_RTE_RING_FULL = -4028;
const int CYPRE_ES_RTE_RING_EMPTY = -4029;
const int CYPRE_ES_PTHREAD_BIND_CORE_ERROR = -4030;
const int CYPRE_ES_CHECKSUM_ERROR = -4031;

// -5000 ~ -5999 SetManager
const int CYPRE_SM_NOT_READY = -5000;
const int CYPRE_SM_RESOURCE_EXHAUSTED = -5001;
const int CYPRE_SM_USER_NOT_FOUND = -5002;

// -6000 ~ -6999 Access
const int CYPRE_AC_SET_NOT_FOUND = -6000;
const int CYPRE_AC_CHANNEL_ERROR = -6001;

}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_ERROR_CODE_H_
