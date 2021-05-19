/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "pb_transfer.h"

namespace cyprestore {
namespace utils {

common::pb::UserStatus ToPbUserStatus(extentmanager::UserStatus status) {
    common::pb::UserStatus pb_status;

    switch (status) {
        case extentmanager::UserStatus::kUserStatusCreated:
            pb_status = common::pb::USER_STATUS_CREATED;
            break;
        case extentmanager::UserStatus::kUserStatusDeleted:
            pb_status = common::pb::USER_STATUS_DELETED;
            break;
        default:
            pb_status = common::pb::USER_STATUS_UNKNOWN;
    }

    return pb_status;
}

bool blobtype_valid(common::pb::BlobType pb_blobtype) {
    bool result = false;
    switch (pb_blobtype) {
        case common::pb::BlobType::BLOB_TYPE_GENERAL:
            result = true;
            break;
        case common::pb::BlobType::BLOB_TYPE_SUPERIOR:
            result = true;
            break;
        case common::pb::BlobType::BLOB_TYPE_EXCELLENT:
            result = true;
            break;
        default:
            break;
    }

    return result;
}

extentmanager::BlobType FromPbBlobType(common::pb::BlobType pb_type) {
    extentmanager::BlobType type;

    switch (pb_type) {
        case common::pb::BlobType::BLOB_TYPE_GENERAL:
            type = extentmanager::BlobType::kBlobTypeGeneral;
            break;
        case common::pb::BlobType::BLOB_TYPE_SUPERIOR:
            type = extentmanager::BlobType::kBlobTypeSuperior;
            break;
        case common::pb::BlobType::BLOB_TYPE_EXCELLENT:
            type = extentmanager::BlobType::kBlobTypeExcellent;
            break;
        default:
            type = extentmanager::BlobType::kBlobTypeUnknown;
            break;
    }

    return type;
}

common::pb::BlobType toPbBlobType(extentmanager::BlobType type) {
    common::pb::BlobType pb_type;
    switch (type) {
        case extentmanager::BlobType::kBlobTypeGeneral:
            pb_type = common::pb::BlobType::BLOB_TYPE_GENERAL;
            break;
        case extentmanager::BlobType::kBlobTypeSuperior:
            pb_type = common::pb::BlobType::BLOB_TYPE_SUPERIOR;
            break;
        case extentmanager::BlobType::kBlobTypeExcellent:
            pb_type = common::pb::BlobType::BLOB_TYPE_EXCELLENT;
            break;
        default:
            pb_type = common::pb::BlobType::BLOB_TYPE_UNKNOWN;
            break;
    }

    return pb_type;
}

common::pb::BlobStatus toPbBlobStatus(extentmanager::BlobStatus status) {
    common::pb::BlobStatus pb_status;
    switch (status) {
        case extentmanager::BlobStatus::kBlobStatusCreated:
            pb_status = common::pb::BlobStatus::BLOB_STATUS_CREATED;
            break;
        case extentmanager::BlobStatus::kBlobStatusDeleted:
            pb_status = common::pb::BlobStatus::BLOB_STATUS_DELETED;
            break;
        case extentmanager::BlobStatus::kBlobStatusResizing:
            pb_status = common::pb::BlobStatus::BLOB_STATUS_RESIZING;
            break;
        case extentmanager::BlobStatus::kBlobStatusSnapshotting:
            pb_status = common::pb::BlobStatus::BLOB_STATUS_SNAPSHOTTING;
            break;
        case extentmanager::BlobStatus::kBlobStatusCloning:
            pb_status = common::pb::BlobStatus::BLOB_STATUS_CLONING;
            break;
        default:
            pb_status = common::pb::BlobStatus::BLOB_STATUS_UNKNOWN;
    }

    return pb_status;
}

extentmanager::PoolType FromPbPoolType(common::pb::PoolType type) {
    extentmanager::PoolType pool_type;
    switch (type) {
        case common::pb::POOL_TYPE_HDD:
            pool_type = extentmanager::PoolType::kPoolTypeHDD;
            break;
        case common::pb::POOL_TYPE_SSD:
            pool_type = extentmanager::PoolType::kPoolTypeSSD;
            break;
        case common::pb::POOL_TYPE_NVME_SSD:
            pool_type = extentmanager::PoolType::kPoolTypeNVMeSSD;
            break;
        case common::pb::POOL_TYPE_SPECIAL:
            pool_type = extentmanager::PoolType::kPoolTypeSpecial;
            break;
        default:
            pool_type = extentmanager::PoolType::kPoolTypeUnknown;
            break;
    }

    return pool_type;
}

extentmanager::PoolFailureDomain
FromPbFailureDomain(common::pb::FailureDomain pb_fd) {
    extentmanager::PoolFailureDomain fd;
    switch (pb_fd) {
        case common::pb::FAILURE_DOMAIN_RACK:
            fd = extentmanager::PoolFailureDomain::kFailureDomainRack;
            break;
        case common::pb::FAILURE_DOMAIN_HOST:
            fd = extentmanager::PoolFailureDomain::kFailureDomainHost;
            break;
        case common::pb::FAILURE_DOMAIN_NODE:
            fd = extentmanager::PoolFailureDomain::kFailureDomainNode;
            break;
        default:
            fd = extentmanager::PoolFailureDomain::kFailureDomainUnknown;
            break;
    }

    return fd;
}

common::pb::PoolType ToPbPoolType(extentmanager::PoolType pool_type) {
    common::pb::PoolType pb_pool_type;
    switch (pool_type) {
        case extentmanager::PoolType::kPoolTypeHDD:
            pb_pool_type = common::pb::POOL_TYPE_HDD;
            break;
        case extentmanager::PoolType::kPoolTypeSSD:
            pb_pool_type = common::pb::POOL_TYPE_SSD;
            break;
        case extentmanager::PoolType::kPoolTypeNVMeSSD:
            pb_pool_type = common::pb::POOL_TYPE_NVME_SSD;
            break;
        default:
            pb_pool_type = common::pb::POOL_TYPE_UNKNOWN;
            break;
    }

    return pb_pool_type;
}

common::pb::PoolStatus ToPbPoolStatus(extentmanager::PoolStatus status) {
    common::pb::PoolStatus pb_status;
    switch (status) {
        case extentmanager::PoolStatus::kPoolStatusCreated:
            pb_status = common::pb::POOL_STATUS_CREATED;
            break;
        case extentmanager::PoolStatus::kPoolStatusEnabled:
            pb_status = common::pb::POOL_STATUS_ENABLED;
            break;
        case extentmanager::PoolStatus::kPoolStatusDisabled:
            pb_status = common::pb::POOL_STATUS_DISABLED;
            break;
        default:
            pb_status = common::pb::POOL_STATUS_UNKNOWN;
            break;
    }

    return pb_status;
}

common::pb::FailureDomain
ToPbFailureDomain(extentmanager::PoolFailureDomain fd) {
    common::pb::FailureDomain pb_fd;
    switch (fd) {
        case extentmanager::PoolFailureDomain::kFailureDomainRack:
            pb_fd = common::pb::FAILURE_DOMAIN_RACK;
            break;
        case extentmanager::PoolFailureDomain::kFailureDomainHost:
            pb_fd = common::pb::FAILURE_DOMAIN_HOST;
            break;
        case extentmanager::PoolFailureDomain::kFailureDomainNode:
            pb_fd = common::pb::FAILURE_DOMAIN_NODE;
            break;
        default:
            pb_fd = common::pb::FAILURE_DOMAIN_UNKNOWN;
            break;
    }

    return pb_fd;
}

bool pool_type_valid(common::pb::PoolType type) {
    bool valid = false;

    switch (type) {
        case common::pb::PoolType::POOL_TYPE_HDD:
            valid = true;
            break;
        case common::pb::PoolType::POOL_TYPE_SSD:
            valid = true;
            break;
        case common::pb::PoolType::POOL_TYPE_NVME_SSD:
            valid = true;
            break;
        default:
            break;
    }

    return valid;
}

bool failure_domain_valid(common::pb::FailureDomain fd) {
    bool valid = false;

    switch (fd) {
        case common::pb::FailureDomain::FAILURE_DOMAIN_RACK:
            valid = true;
            break;
        case common::pb::FailureDomain::FAILURE_DOMAIN_HOST:
            valid = true;
            break;
        case common::pb::FailureDomain::FAILURE_DOMAIN_NODE:
            valid = true;
            break;
        default:
            break;
    }

    return valid;
}

common::pb::ESStatus ToPbExtentServerStatus(extentmanager::ESStatus status) {
    common::pb::ESStatus pb_status;
    switch (status) {
        case extentmanager::ESStatus::kESStatusOk:
            pb_status = common::pb::ESStatus::ES_STATUS_OK;
            break;
        case extentmanager::ESStatus::kESStatusFailed:
            pb_status = common::pb::ESStatus::ES_STATUS_FAILED;
            break;
        case extentmanager::ESStatus::kESStatusRecovering:
            pb_status = common::pb::ESStatus::ES_STATUS_RECOVERING;
            break;
        case extentmanager::ESStatus::kESStatusRebalancing:
            pb_status = common::pb::ESStatus::ES_STATUS_REBALANCING;
            break;
        default:
            pb_status = common::pb::ESStatus::ES_STATUS_UNKNOWN;
            break;
    }

    return pb_status;
}

common::pb::RGStatus ToPbRgStatus(const extentmanager::RGStatus status) {
    common::pb::RGStatus pb_status;
    switch (status) {
        case extentmanager::RGStatus::kRGStatusOk:
            pb_status = common::pb::RGStatus::RG_STATUS_OK;
            break;
        case extentmanager::RGStatus::kRGStatusDowngrade:
            pb_status = common::pb::RGStatus::RG_STATUS_DOWNGRADE;
            break;
        case extentmanager::RGStatus::kRGStatusRecovering:
            pb_status = common::pb::RGStatus::RG_STATUS_RECOVERING;
            break;
        case extentmanager::RGStatus::kRGStatusRebalancing:
            pb_status = common::pb::RGStatus::RG_STATUS_REBALANCING;
            break;
        default:
            pb_status = common::pb::RGStatus::RG_STATUS_UNKNOWN;
            break;
    }

    return pb_status;
}

common::pb::SetRoute ToPbSetRoute(const common::pb::Set &set) {
    common::pb::SetRoute route;
    route.set_set_id(set.id());
    route.set_set_name(set.name());
    route.set_ip(set.em().endpoint().public_ip());
    route.set_port(set.em().endpoint().public_port());
    return route;
}

// From
/*
common::ExtentRouterPtr FromPbExtentRouter(const common::pb::ExtentRouter
&pb_router) { common::ExtentRouterPtr router =
std::make_shared<common::ExtentRouter>();

    router->version = pb_router.version();
    router->extent_id = pb_router.extent_id();

    common::EsInstancePtr primary =
std::make_shared<common::EsInstance>(pb_router.primary().es_id());
    primary->public_ip = pb_router.primary().public_ip();
    primary->public_port = pb_router.primary().public_port();
    primary->private_ip = pb_router.primary().private_ip();
    primary->private_port = pb_router.primary().private_port();

    for (int i = 0; i < pb_router.secondaries_size(); i++) {
        common::EsInstancePtr secondary =
std::make_shared<common::EsInstance>(pb_router.secondaries(i).es_id());
        secondary->public_ip = pb_router.secondaries(i).public_ip();
        secondary->public_port = pb_router.secondaries(i).public_port();
        secondary->private_ip = pb_router.secondaries(i).private_ip();
        secondary->private_port = pb_router.secondaries(i).private_port();

        router->secondaries.push_back(secondary);
    }

    return router;
}
*/

}  // namespace utils
}  // namespace cyprestore
