/*
 * Copyright 2020 JDD authors.
 * @chenjianfei18
 *
 */

#include "libcypre_c.h"

#include <stdint.h>
#include <unistd.h>

#include <condition_variable>
#include <mutex>

#include "common/error_code.h"
#include "libcypre.h"
#include "stream/rbd_stream_handle.h"

using namespace cyprestore::clients;

extern "C" {

struct cyprerbd {
    CypreRBD *rbd_handle;
};

struct cypreblob {
    RBDStreamHandlePtr blob_handle;
};

cyprerbd_t *open_cyprerbd(const char *em_ip, int em_port) {
    cyprerbd_t *rbd;
    CypreRBDOptions opt(std::string(em_ip), em_port);

    rbd = new cyprerbd_t;
    rbd->rbd_handle = CypreRBD::New();
    if (!rbd->rbd_handle) {
        delete rbd;
        return NULL;
    }

    if (rbd->rbd_handle->Init(opt) != cyprestore::common::CYPRE_OK) {
        close_cyprerbd(rbd);
        return NULL;
    }
    return rbd;
}

void close_cyprerbd(cyprerbd_t *rbd) {
    delete rbd->rbd_handle;
    delete rbd;
}

cypreblob_t *open_blob(cyprerbd_t *rbd, const char *blob_id) {
    cypreblob_t *blob = new cypreblob_t;
    if (rbd->rbd_handle->Open(std::string(blob_id), blob->blob_handle)
        != cyprestore::common::CYPRE_OK) {
        delete blob;
        return NULL;
    }
    return blob;
}

void close_blob(cyprerbd_t *rbd, cypreblob_t *blob) {
    rbd->rbd_handle->Close(blob->blob_handle);
}

uint64_t get_blob_size(cypreblob_t *blob) {
    return blob->blob_handle->GetDeviceSize();
}

int read_blob(cypreblob_t *blob, char *buf, uint32_t len, uint64_t offset) {
    if (blob->blob_handle->Read((void *)buf, len, offset)
        != cyprestore::common::CYPRE_OK) {
        return -1;
    }
    return 0;
}

int write_blob(
        cypreblob_t *blob, const char *buf, uint32_t len, uint64_t offset) {
    if (blob->blob_handle->Write((void *)buf, len, offset)
        != cyprestore::common::CYPRE_OK) {
        return -1;
    }
    return 0;
}

struct aio_completion {
    bool is_done;  // is io completed
    int rc;        // result code: 0 = succ, otherwise = fail
};

aio_completion_t *aio_create_completion() {
    return new aio_completion_t;
}

void aio_release_completion(aio_completion_t *c) {
    delete c;
}

bool aio_is_complete(aio_completion_t *c) {
    return c->is_done;
}

bool aio_is_success(aio_completion_t *c) {
    return c->rc == cyprestore::common::CYPRE_OK;
}

void aio_wait_for_complete(aio_completion_t *c) {
    while (!c->is_done) {
        usleep(1);
    }
}

static void aio_inner_cb(int rc, void *ctx) {
    aio_completion_t *c = (aio_completion_t *)ctx;
    c->rc = rc;
    c->is_done = true;
}

int aio_read_blob(
        cypreblob_t *blob, char *buf, uint32_t len, uint64_t offset,
        aio_completion_t *c) {
    if (c == NULL) {
        return -1;
    }
    c->is_done = false;
    if (blob->blob_handle->AsyncRead(buf, len, offset, aio_inner_cb, c)
        != cyprestore::common::CYPRE_OK) {
        return -1;
    }
    return 0;
}

int aio_write_blob(
        cypreblob_t *blob, const char *buf, uint32_t len, uint64_t offset,
        aio_completion_t *c) {
    if (c == NULL) {
        return -1;
    }
    c->is_done = false;
    if (blob->blob_handle->AsyncWrite(buf, len, offset, aio_inner_cb, c)
        != cyprestore::common::CYPRE_OK) {
        return -1;
    }
    return 0;
}

}  // end extern "C"
