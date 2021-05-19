/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_UTILS_PB_TRANSFER_H_
#define CYPRESTORE_UTILS_PB_TRANSFER_H_

#include "common/pb/types.pb.h"
#include "extentmanager/enum.h"

namespace cyprestore {
namespace utils {

common::pb::UserStatus ToPbUserStatus(extentmanager::UserStatus status);

bool blobtype_valid(cyprestore::common::pb::BlobType blob_type);

extentmanager::BlobType FromPbBlobType(common::pb::BlobType pb_type);

common::pb::BlobStatus toPbBlobStatus(extentmanager::BlobStatus status);

common::pb::BlobType toPbBlobType(extentmanager::BlobType type);

extentmanager::PoolType FromPbPoolType(common::pb::PoolType type);

extentmanager::PoolFailureDomain
FromPbFailureDomain(common::pb::FailureDomain pb_fd);

common::pb::PoolType ToPbPoolType(extentmanager::PoolType pool_type);

common::pb::PoolStatus ToPbPoolStatus(extentmanager::PoolStatus status);

common::pb::FailureDomain
ToPbFailureDomain(extentmanager::PoolFailureDomain fd);

bool pool_type_valid(common::pb::PoolType type);

bool failure_domain_valid(common::pb::FailureDomain fd);

bool replica_count_valid(int replica_count);

common::pb::ESStatus ToPbExtentServerStatus(extentmanager::ESStatus status);

common::pb::RGStatus ToPbRgStatus(const extentmanager::RGStatus status);

common::pb::SetRoute ToPbSetRoute(const common::pb::Set &set);

// From
// std::shared_ptr<extentmanager::ExtentRouter> FromPbExtentRouter(const
// common::pb::ExtentRouter &pb_router);

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_PB_TRANSFER_H_
