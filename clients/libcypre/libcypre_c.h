/*
 * Copyright 2020 JDD authors.
 * @chenjianfei18
 *
 */

#ifndef CYPRESTORE_LIBCYPRE_C_H_
#define CYPRESTORE_LIBCYPRE_C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct cyprerbd cyprerbd_t;
typedef struct cypreblob cypreblob_t;
typedef struct aio_completion aio_completion_t;

// rbd api.
cyprerbd_t *open_cyprerbd(const char *em_ip, int em_port);
void close_cyprerbd(cyprerbd_t *rbd);

// blob api.
cypreblob_t *open_blob(cyprerbd_t *rbd, const char *blob_id);
void close_blob(cyprerbd_t *rbd, cypreblob_t *blob);
uint64_t get_blob_size(cypreblob_t *blob);

// sync rw blob api.
int read_blob(cypreblob_t *blob, char *buf, uint32_t len, uint64_t offset);
int write_blob(
        cypreblob_t *blob, const char *buf, uint32_t len, uint64_t offset);

// async rw blob api.
int aio_read_blob(
        cypreblob_t *blob, char *buf, uint32_t len, uint64_t offset,
        aio_completion_t *c);
int aio_write_blob(
        cypreblob_t *blob, const char *buf, uint32_t len, uint64_t offset,
        aio_completion_t *c);

aio_completion_t *aio_create_completion();
void aio_release_completion(aio_completion_t *c);

bool aio_is_complete(aio_completion_t *c);
bool aio_is_success(aio_completion_t *c);
void aio_wait_for_complete(aio_completion_t *c);

#ifdef __cplusplus
}  // end extern "C"
#endif

#endif  // CYPRESTORE_LIBCYPRE_C_H_
