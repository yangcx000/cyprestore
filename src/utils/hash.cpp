/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "hash.h"

#include "stdint.h"

namespace cyprestore {
namespace utils {

unsigned int HashUtil::mur_mur_hash(const void *key, int len) {
    return mur_mur_hash(key, len, 97);
}

unsigned int
HashUtil::mur_mur_hash(const void *key, int len, unsigned int seed) {
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value
    unsigned int h = seed ^ len;

    // Mix 4 bytes at a time into the hash
    const unsigned char *data = (const unsigned char *)key;

    while (len >= 4) {
        unsigned int k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array
    const int8_t *idata = (const int8_t *)data;
    switch (len) {
        case 3:
            h ^= idata[2] << 16;
        case 2:
            h ^= idata[1] << 8;
        case 1:
            h ^= idata[0];
            h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

}  // namespace utils
}  // namespace cyprestore
