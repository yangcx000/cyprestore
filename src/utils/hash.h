/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_UTILS_HASH_H_
#define CYPRESTORE_UTILS_HASH_H_

namespace cyprestore {
namespace utils {

class HashUtil {
public:
    static unsigned int mur_mur_hash(const void *key, int len);

private:
    static unsigned int
    mur_mur_hash(const void *key, int len, unsigned int seed);
};

}  // namespace utils
}  // namespace cyprestore

#endif  // CYPRESTORE_UTILS_HASH_H_
