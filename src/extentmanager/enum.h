/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTMANAGER_ENUM_H_
#define CYPRESTORE_EXTENTMANAGER_ENUM_H_

namespace cyprestore {
namespace extentmanager {

/*
 * pool
 */
enum PoolType {
    kPoolTypeHDD = 0,
    kPoolTypeSSD,
    kPoolTypeNVMeSSD,
    kPoolTypeSpecial,
    kPoolTypeUnknown = -1,
};

enum PoolStatus {
    kPoolStatusCreated = 0,
    kPoolStatusInitializing,
    kPoolStatusEnabled,
    kPoolStatusDisabled,
    kPoolStatusUnknown = -1,
};

enum PoolFailureDomain {
    kFailureDomainRack = 0,
    kFailureDomainHost,
    kFailureDomainNode,
    kFailureDomainUnknown = -1,
};

/*
 * user
 */
enum UserStatus {
    kUserStatusCreated = 0,
    kUserStatusDeleted,
    kUserStatusUnknown = -1,
};

/*
 * extent server
 */
enum ESStatus {
    kESStatusOk = 0,
    kESStatusFailed,
    kESStatusRecovering,
    kESStatusRebalancing,
    kESStatusUnknown = -1,
};

/*
 * blob
 */
enum BlobType {
    kBlobTypeGeneral = 0,
    kBlobTypeSuperior,
    kBlobTypeExcellent,
    kBlobTypeUnknown = -1,
};

enum BlobStatus {
    kBlobStatusCreated = 0,
    kBlobStatusDeleted,
    kBlobStatusResizing,
    kBlobStatusSnapshotting,
    kBlobStatusCloning,
    kBlobStatusUnknown = -1,
};

/*
 * replication group
 */
enum RGStatus {
    kRGStatusOk = 0,
    kRGStatusDowngrade,
    kRGStatusRecovering,
    kRGStatusRebalancing,
    kRGStatusUnknown = -1,
};

}  // namespace extentmanager
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTMANAGER_ENUM_H_
