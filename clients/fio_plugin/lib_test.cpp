/*
 * Copyright 2020 JDD authors.
 * @chenjianfei18
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include "libcypre_c.h"

#define BLOCK_SIZE 4096

using namespace std;

void assert_true(bool ok, const char *op);
void check_data(const char *s1, const char *s2, uint64_t len);

int main(int argc, char **argv) {
    if (argc < 4) {
        cout << "args error: ./exe [em-ip] [em-port] [blob-id]" << endl;
        exit(1);
    }
    char *em_ip = argv[1];
    int em_port = atoi(argv[2]);
    char *blob_id = argv[3];

    cout << "em-ip = " << em_ip << endl;
    cout << "em-port = " << em_port << endl;
    cout << "blob-id = " << blob_id << endl << endl;

    // open rbd and blob.
    cyprerbd_t *rbd = open_cyprerbd(em_ip, em_port);
    assert_true(rbd != NULL, "open_cyprerbd");
    cypreblob_t *blob = open_blob(rbd, blob_id);
    assert_true(blob != NULL, "open_blob");

    // sync wirte.
    uint64_t offset = 0;
    const char input[BLOCK_SIZE + 1] = { 'a' };
    char output[BLOCK_SIZE + 1] = { 0 };
    int ret = write_blob(blob, input, BLOCK_SIZE, offset);
    assert_true(ret == 0, "write_blob");

    // sync read.
    ret = read_blob(blob, output, BLOCK_SIZE, offset);
    assert_true(ret == 0, "read_blob");
    check_data(input, output, BLOCK_SIZE);

    // async write.
    aio_completion_t *c = aio_create_completion();
    offset = BLOCK_SIZE;
    ret = aio_write_blob(blob, input, BLOCK_SIZE, offset, c);
    assert_true(ret == 0, "aio_write_blob");
    aio_wait_for_complete(c);
    assert_true(aio_is_success(c), "aio write complete");
    aio_release_completion(c);

    // async read.
    c = aio_create_completion();
    memset(output, 0, BLOCK_SIZE + 1);
    ret = aio_read_blob(blob, output, BLOCK_SIZE, offset, c);
    assert_true(ret == 0, "aio_read_blob");
    aio_wait_for_complete(c);
    assert_true(aio_is_success(c), "aio read complete");
    aio_release_completion(c);

    // close blob and rbd.
    close_blob(rbd, blob);
    assert_true(true, "close_blob");
    close_cyprerbd(rbd);
    assert_true(true, "close_cyprerbd");

    return 0;
}

void assert_true(bool ok, const char *op) {
    if (ok) {
        cout << op << " succ" << endl;
    } else {
        cout << op << " fail" << endl;
        exit(1);
    }
}

void check_data(const char *s1, const char *s2, uint64_t len) {
    if (strncmp(s1, s2, len) == 0) {
        cout << "data check succ" << endl;
    } else {
        cout << "data check fail" << endl;
        exit(1);
    }
}
