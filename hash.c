#include "hash.h"
#include "crc32c.h"
#include "xxhash.h"
#include "blake2.h"
#include "blake3.h"

void blake3_hasher_update_portable(blake3_hasher *self, const void *input,
		size_t input_len);
void blake3_hasher_update_sse41(blake3_hasher *self, const void *input,
		size_t input_len);
void blake3_hasher_update_avx2(blake3_hasher *self, const void *input,
		size_t input_len);

int hash_crc32c(const u8* buf, size_t length, u8 *out)
{
	u32 crc = ~0;

	crc = crc32c(~0, buf, length);
	put_unaligned_le32(~crc, out);

	return 0;
}

int hash_xxhash(const u8 *buf, size_t length, u8 *out)
{
	XXH64_hash_t hash;

	hash = XXH64(buf, length, 0);
	/*
	 * NOTE: we're not taking the canonical form here but the plain hash to
	 * be compatible with the kernel implementation!
	 */
	memcpy(out, &hash, 8);

	return 0;
}

int hash_blake2b(const u8 *buf, size_t len, u8 *out)
{
	blake2b_state S;

	blake2b_init(&S, CRYPTO_HASH_SIZE_MAX);
	blake2b_update(&S, buf, len);
	blake2b_final(&S, out, CRYPTO_HASH_SIZE_MAX);

	return 0;
}

int hash_blake3(const u8 *buf, size_t len, u8 *out)
{
	blake3_hasher state;

	blake3_hasher_init(&state);
	blake3_hasher_update_portable(&state, buf, len);
	blake3_hasher_finalize(&state, out, CRYPTO_HASH_SIZE_MAX);

	return 0;
}

int hash_blake3_sse41(const u8 *buf, size_t len, u8 *out)
{
	blake3_hasher state;

	blake3_hasher_init(&state);
	blake3_hasher_update_sse41(&state, buf, len);
	blake3_hasher_finalize(&state, out, CRYPTO_HASH_SIZE_MAX);

	return 0;
}

int hash_blake3_avx2(const u8 *buf, size_t len, u8 *out)
{
	blake3_hasher state;

	blake3_hasher_init(&state);
	blake3_hasher_update_avx2(&state, buf, len);
	blake3_hasher_finalize(&state, out, CRYPTO_HASH_SIZE_MAX);

	return 0;
}
