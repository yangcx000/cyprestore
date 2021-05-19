/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
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
