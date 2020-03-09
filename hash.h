#ifndef CRYPTO_HASH_H
#define CRYPTO_HASH_H

#include "compat.h"

#define CRYPTO_HASH_SIZE_MAX	32

int hash_crc32c(const u8 *buf, size_t length, u8 *out);
int hash_xxhash(const u8 *buf, size_t length, u8 *out);
int hash_blake2b(const u8 *buf, size_t length, u8 *out);
int hash_blake3(const u8 *buf, size_t length, u8 *out);
int hash_blake3_sse41(const u8 *buf, size_t length, u8 *out);
int hash_blake3_avx2(const u8 *buf, size_t length, u8 *out);

#endif
