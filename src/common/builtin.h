/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_COMMON_BUILTIN_H_
#define CYPRESTORE_COMMON_BUILTIN_H_

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#if (__i386__ || __i386 || __amd64__ || __amd64)
#define cypres_cpu_pause() __asm__("pause")
#else
#define cypres_cpu_pause()
#endif

namespace cyprestore {
namespace common {}  // namespace common
}  // namespace cyprestore

#endif  // CYPRESTORE_COMMON_BUILTIN_H_
