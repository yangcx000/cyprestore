/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "libcypre.h"

#include "cypre_cluster_rbd.h"

using namespace cyprestore;
using namespace cyprestore::clients;

CypreRBD *CypreRBD::New() {
    return new CypreClusterRBD();
}
