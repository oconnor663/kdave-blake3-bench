#include "hash.h"
#include "crc32c.h"
#include "xxhash.h"
#include "blake2.h"
#include "blake2b.h"
#include "blake3.h"

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
    // A BLAKE2b hash is 64 bytes long. This implementation doesn't support the
    // BLAKE2 output length parameter, so for this benchmark we just truncate it.
    u8 wide_out[BLAKE2B_OUTBYTES];
    blake2b(wide_out, buf, len);
    memcpy(out, wide_out, CRYPTO_HASH_SIZE_MAX);

	return 0;
}

int hash_blake3(const u8 *buf, size_t len, u8 *out)
{
	blake3_hasher state;

	blake3_hasher_init(&state);
	blake3_hasher_update(&state, buf, len);
	blake3_hasher_finalize(&state, out, CRYPTO_HASH_SIZE_MAX);

	return 0;
}
