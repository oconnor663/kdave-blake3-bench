#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if __POPCNT__
#include <nmmintrin.h>
#endif

#include "blake3.h"

// internal flags
#define CHUNK_START 1
#define CHUNK_END 2
#define PARENT 4
#define ROOT 8
#define KEYED_HASH 16
#define DERIVE_KEY_CONTEXT 32
#define DERIVE_KEY_MATERIAL 64

// This C implementation tries to support recent versions of GCC, Clang, and
// MSVC.
#if defined(_MSC_VER)
#define INLINE __forceinline static
#else
#define INLINE __attribute__((always_inline)) static inline
#endif

static const uint32_t IV[8] = {0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL,
                               0xA54FF53AUL, 0x510E527FUL, 0x9B05688CUL,
                               0x1F83D9ABUL, 0x5BE0CD19UL};

static const uint8_t MSG_SCHEDULE[7][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8},
    {3, 4, 10, 12, 13, 2, 7, 14, 6, 5, 9, 0, 11, 15, 8, 1},
    {10, 7, 12, 9, 14, 3, 13, 15, 4, 0, 11, 2, 5, 8, 1, 6},
    {12, 13, 9, 11, 15, 10, 14, 8, 7, 2, 5, 3, 0, 1, 6, 4},
    {9, 14, 11, 5, 8, 12, 15, 1, 13, 3, 0, 10, 2, 6, 4, 7},
    {11, 15, 5, 0, 1, 9, 8, 6, 14, 10, 2, 12, 3, 4, 7, 13},
};

// Count the number of 1 bits.
INLINE uint8_t popcnt(uint64_t x) {
#if __POPCNT__
  return (uint8_t)_mm_popcnt_u64(x);
#else
  uint8_t count = 0;
  while (x > 0) {
    count += ((uint8_t)x) & 1;
    x >>= 1;
  }
  return count;
#endif
}

INLINE uint32_t counter_low(uint64_t counter) { return (uint32_t)counter; }

INLINE uint32_t counter_high(uint64_t counter) {
  return (uint32_t)(counter >> 32);
}

INLINE uint32_t load32(const void *src) {
  const uint8_t *p = (const uint8_t *)src;
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
}

INLINE void load_key_words(const uint8_t key[BLAKE3_KEY_LEN],
                           uint32_t key_words[8]) {
  key_words[0] = load32(&key[0 * 4]);
  key_words[1] = load32(&key[1 * 4]);
  key_words[2] = load32(&key[2 * 4]);
  key_words[3] = load32(&key[3 * 4]);
  key_words[4] = load32(&key[4 * 4]);
  key_words[5] = load32(&key[5 * 4]);
  key_words[6] = load32(&key[6 * 4]);
  key_words[7] = load32(&key[7 * 4]);
}

void blake3_compress_in_place(uint32_t cv[8],
                              const uint8_t block[BLAKE3_BLOCK_LEN],
                              uint8_t block_len, uint64_t counter,
                              uint8_t flags);

void blake3_compress_xof(const uint32_t cv[8],
                         const uint8_t block[BLAKE3_BLOCK_LEN],
                         uint8_t block_len, uint64_t counter, uint8_t flags,
                         uint8_t out[64]);

void blake3_hash_many(const uint8_t *const *inputs, size_t num_inputs,
                      size_t blocks, const uint32_t key[8], uint64_t counter,
                      bool increment_counter, uint8_t flags,
                      uint8_t flags_start, uint8_t flags_end, uint8_t *out);

typedef struct {
	uint32_t input_cv[8];
	uint64_t counter;
	uint8_t block[BLAKE3_BLOCK_LEN];
	uint8_t block_len;
	uint8_t flags;
} output_t;

INLINE void chunk_state_init(blake3_chunk_state *self, const uint32_t key[8],
		uint8_t flags) {
	memcpy(self->cv, key, BLAKE3_KEY_LEN);
	self->chunk_counter = 0;
	memset(self->buf, 0, BLAKE3_BLOCK_LEN);
	self->buf_len = 0;
	self->blocks_compressed = 0;
	self->flags = flags;
}

INLINE void chunk_state_reset(blake3_chunk_state *self, const uint32_t key[8],
		uint64_t chunk_counter) {
	memcpy(self->cv, key, BLAKE3_KEY_LEN);
	self->chunk_counter = chunk_counter;
	self->blocks_compressed = 0;
	memset(self->buf, 0, BLAKE3_BLOCK_LEN);
	self->buf_len = 0;
}

INLINE output_t make_output(const uint32_t input_cv[8],
		const uint8_t block[BLAKE3_BLOCK_LEN],
		uint8_t block_len, uint64_t counter,
		uint8_t flags) {
	output_t ret;
	memcpy(ret.input_cv, input_cv, 32);
	memcpy(ret.block, block, BLAKE3_BLOCK_LEN);
	ret.block_len = block_len;
	ret.counter = counter;
	ret.flags = flags;
	return ret;
}

INLINE size_t chunk_state_fill_buf(blake3_chunk_state *self,
		const uint8_t *input, size_t input_len) {
	size_t take = BLAKE3_BLOCK_LEN - ((size_t)self->buf_len);
	if (take > input_len) {
		take = input_len;
	}
	uint8_t *dest = self->buf + ((size_t)self->buf_len);
	memcpy(dest, input, take);
	self->buf_len += (uint8_t)take;
	return take;
}

INLINE uint8_t chunk_state_maybe_start_flag(const blake3_chunk_state *self) {
	if (self->blocks_compressed == 0) {
		return CHUNK_START;
	} else {
		return 0;
	}
}

INLINE output_t chunk_state_output(const blake3_chunk_state *self) {
	uint8_t block_flags =
		self->flags | chunk_state_maybe_start_flag(self) | CHUNK_END;
	return make_output(self->cv, self->buf, self->buf_len, self->chunk_counter,
			block_flags);
}

INLINE size_t chunk_state_len(const blake3_chunk_state *self) {
	return (BLAKE3_BLOCK_LEN * (size_t)self->blocks_compressed) +
		((size_t)self->buf_len);
}

// Chaining values within a given chunk (specifically the compress_in_place
// interface) are represented as words. This avoids unnecessary bytes<->words
// conversion overhead in the portable implementation. However, the hash_many
// interface handles both user input and parent node blocks, so it accepts
// bytes. For that reason, chaining values in the CV stack are represented as
// bytes.
INLINE void output_chaining_value(const output_t *self, uint8_t cv[32]) {
	uint32_t cv_words[8];
	memcpy(cv_words, self->input_cv, 32);
	blake3_compress_in_place(cv_words, self->block, self->block_len, self->counter,
			self->flags);
	memcpy(cv, cv_words, 32);
}

INLINE void output_root_bytes(const output_t *self, uint8_t *out,
		size_t out_len) {
	uint64_t output_block_counter = 0;
	uint8_t wide_buf[64];
	while (out_len > 0) {
		blake3_compress_xof(self->input_cv, self->block, self->block_len,
				output_block_counter, self->flags | ROOT, wide_buf);
		size_t memcpy_len;
		if (out_len > 64) {
			memcpy_len = 64;
		} else {
			memcpy_len = out_len;
		}
		memcpy(out, wide_buf, memcpy_len);
		out += memcpy_len;
		out_len -= memcpy_len;
		output_block_counter += 1;
	}
}

INLINE void chunk_state_update(blake3_chunk_state *self, const uint8_t *input,
		size_t input_len) {
	if (self->buf_len > 0) {
		size_t take = chunk_state_fill_buf(self, input, input_len);
		input += take;
		input_len -= take;
		if (input_len > 0) {
			blake3_compress_in_place(self->cv, self->buf, BLAKE3_BLOCK_LEN,
					self->chunk_counter,
					self->flags | chunk_state_maybe_start_flag(self));
			self->blocks_compressed += 1;
			self->buf_len = 0;
			memset(self->buf, 0, BLAKE3_BLOCK_LEN);
		}
	}

	while (input_len > BLAKE3_BLOCK_LEN) {
		blake3_compress_in_place(self->cv, input, BLAKE3_BLOCK_LEN, self->chunk_counter,
				self->flags | chunk_state_maybe_start_flag(self));
		self->blocks_compressed += 1;
		input += BLAKE3_BLOCK_LEN;
		input_len -= BLAKE3_BLOCK_LEN;
	}

	size_t take = chunk_state_fill_buf(self, input, input_len);
	input += take;
	input_len -= take;
}

INLINE output_t parent_output(const uint8_t block[BLAKE3_BLOCK_LEN],
		const uint32_t key[8], uint8_t flags) {
	return make_output(key, block, BLAKE3_BLOCK_LEN, 0, flags | PARENT);
}

INLINE void hasher_init_base(blake3_hasher *self, const uint32_t key[8],
		uint8_t flags) {
	memcpy(self->key, key, BLAKE3_KEY_LEN);
	chunk_state_init(&self->chunk, key, flags);
	self->cv_stack_len = 0;
}

INLINE bool hasher_needs_merge(const blake3_hasher *self,
		uint64_t total_chunks) {
	return self->cv_stack_len > popcnt(total_chunks);
}

INLINE void hasher_merge_parent(blake3_hasher *self) {
	size_t parent_block_start =
		(((size_t)self->cv_stack_len) - 2) * BLAKE3_OUT_LEN;
	output_t output = parent_output(&self->cv_stack[parent_block_start],
			self->key, self->chunk.flags);
	output_chaining_value(&output, &self->cv_stack[parent_block_start]);
	self->cv_stack_len -= 1;
}

INLINE void hasher_push_chunk_cv(blake3_hasher *self,
		uint8_t cv[BLAKE3_OUT_LEN],
		uint64_t chunk_counter) {
	assert(self->cv_stack_len < BLAKE3_MAX_DEPTH);
	while (hasher_needs_merge(self, chunk_counter)) {
		hasher_merge_parent(self);
	}
	memcpy(&self->cv_stack[self->cv_stack_len * BLAKE3_OUT_LEN], cv,
			BLAKE3_OUT_LEN);
	self->cv_stack_len += 1;
}

