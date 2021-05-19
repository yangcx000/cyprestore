/*******************************************************************************
  Copyright (c) 2017-2018, Intel Corporation

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of Intel Corporation nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef IMB_DES_H
#define IMB_DES_H

#include <stdint.h>

/**
 * @brief DES CBC encryption
 *
 * @param input source buffer with plain text
 * @param output destination buffer for cipher text
 * @param size number of bytes to encrypt (multiple of 8)
 * @param ks pointer to key schedule structure
 * @param ivec pointer to initialization vector
 */
void des_enc_cbc_basic(const void *input, void *output, const int size,
                       const uint64_t *ks, const uint64_t *ivec);

/**
 * @brief DES CBC decryption
 *
 * @param input source buffer with cipher text
 * @param output destination buffer for plain text
 * @param size number of bytes to decrypt (multiple of 8)
 * @param ks pointer to key schedule structure
 * @param ivec pointer to initialization vector
 */
void des_dec_cbc_basic(const void *input, void *output, const int size,
                       const uint64_t *ks, const uint64_t *ivec);

/**
 * @brief 3DES CBC encryption
 *
 * @param input source buffer with plain text
 * @param output destination buffer for cipher text
 * @param size number of bytes to encrypt (multiple of 8)
 * @param ks1 pointer to key schedule 1 structure
 * @param ks2 pointer to key schedule 2 structure
 * @param ks3 pointer to key schedule 3 structure
 * @param ivec pointer to initialization vector
 */
void des3_enc_cbc_basic(const void *input, void *output, const int size,
                        const uint64_t *ks1, const uint64_t *ks2,
                        const uint64_t *ks3, const uint64_t *ivec);

/**
 * @brief 3DES CBC decryption
 *
 * @param input source buffer with cipher text
 * @param output destination buffer for plain text
 * @param size number of bytes to decrypt (multiple of 8)
 * @param ks1 pointer to key schedule 1 structure
 * @param ks2 pointer to key schedule 2 structure
 * @param ks3 pointer to key schedule 3 structure
 * @param ivec pointer to initialization vector
 */
void des3_dec_cbc_basic(const void *input, void *output, const int size,
                        const uint64_t *ks1, const uint64_t *ks2,
                        const uint64_t *ks3, const uint64_t *ivec);

/**
 * @brief DOCSIS DES encryption
 *
 * @param input source buffer with plain text
 * @param output destination buffer for cipher text
 * @param size number of bytes to encrypt
 * @param ks pointer to key schedule structure
 * @param ivec pointer to initialization vector
 */
void docsis_des_enc_basic(const void *input, void *output, const int size,
                          const uint64_t *ks, const uint64_t *ivec);

/**
 * @brief DOCSIS DES decryption
 *
 * @param input source buffer with cipher text
 * @param output destination buffer for plain text
 * @param size number of bytes to decrypt
 * @param ks pointer to key schedule structure
 * @param ivec pointer to initialization vector
 */
void docsis_des_dec_basic(const void *input, void *output, const int size,
                          const uint64_t *ks, const uint64_t *ivec);

#endif /* IMB_DES_H */
