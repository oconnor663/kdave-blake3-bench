#include "blake3_impl.h"
#include <string.h>

INLINE void store32(void *dst, uint32_t w) {
  uint8_t *p = (uint8_t *)dst;
  p[0] = (uint8_t)(w >> 0);
  p[1] = (uint8_t)(w >> 8);
  p[2] = (uint8_t)(w >> 16);
  p[3] = (uint8_t)(w >> 24);
}

INLINE uint32_t rotr32(uint32_t w, uint32_t c) {
  return (w >> c) | (w << (32 - c));
}

INLINE void g(uint32_t *state, size_t a, size_t b, size_t c, size_t d,
              uint32_t x, uint32_t y) {
  state[a] = state[a] + state[b] + x;
  state[d] = rotr32(state[d] ^ state[a], 16);
  state[c] = state[c] + state[d];
  state[b] = rotr32(state[b] ^ state[c], 12);
  state[a] = state[a] + state[b] + y;
  state[d] = rotr32(state[d] ^ state[a], 8);
  state[c] = state[c] + state[d];
  state[b] = rotr32(state[b] ^ state[c], 7);
}

INLINE void round_fn(uint32_t state[16], const uint32_t *msg, size_t round) {
  // Select the message schedule based on the round.
  const uint8_t *schedule = MSG_SCHEDULE[round];

  // Mix the columns.
  g(state, 0, 4, 8, 12, msg[schedule[0]], msg[schedule[1]]);
  g(state, 1, 5, 9, 13, msg[schedule[2]], msg[schedule[3]]);
  g(state, 2, 6, 10, 14, msg[schedule[4]], msg[schedule[5]]);
  g(state, 3, 7, 11, 15, msg[schedule[6]], msg[schedule[7]]);

  // Mix the rows.
  g(state, 0, 5, 10, 15, msg[schedule[8]], msg[schedule[9]]);
  g(state, 1, 6, 11, 12, msg[schedule[10]], msg[schedule[11]]);
  g(state, 2, 7, 8, 13, msg[schedule[12]], msg[schedule[13]]);
  g(state, 3, 4, 9, 14, msg[schedule[14]], msg[schedule[15]]);
}

INLINE void compress_pre(uint32_t state[16], const uint32_t cv[8],
                         const uint8_t block[BLAKE3_BLOCK_LEN],
                         uint8_t block_len, uint64_t counter, uint8_t flags) {
  uint32_t block_words[16];
  block_words[0] = load32(block + 4 * 0);
  block_words[1] = load32(block + 4 * 1);
  block_words[2] = load32(block + 4 * 2);
  block_words[3] = load32(block + 4 * 3);
  block_words[4] = load32(block + 4 * 4);
  block_words[5] = load32(block + 4 * 5);
  block_words[6] = load32(block + 4 * 6);
  block_words[7] = load32(block + 4 * 7);
  block_words[8] = load32(block + 4 * 8);
  block_words[9] = load32(block + 4 * 9);
  block_words[10] = load32(block + 4 * 10);
  block_words[11] = load32(block + 4 * 11);
  block_words[12] = load32(block + 4 * 12);
  block_words[13] = load32(block + 4 * 13);
  block_words[14] = load32(block + 4 * 14);
  block_words[15] = load32(block + 4 * 15);

  state[0] = cv[0];
  state[1] = cv[1];
  state[2] = cv[2];
  state[3] = cv[3];
  state[4] = cv[4];
  state[5] = cv[5];
  state[6] = cv[6];
  state[7] = cv[7];
  state[8] = IV[0];
  state[9] = IV[1];
  state[10] = IV[2];
  state[11] = IV[3];
  state[12] = counter_low(counter);
  state[13] = counter_high(counter);
  state[14] = (uint32_t)block_len;
  state[15] = (uint32_t)flags;

  round_fn(state, &block_words[0], 0);
  round_fn(state, &block_words[0], 1);
  round_fn(state, &block_words[0], 2);
  round_fn(state, &block_words[0], 3);
  round_fn(state, &block_words[0], 4);
  round_fn(state, &block_words[0], 5);
  round_fn(state, &block_words[0], 6);
}

void blake3_compress_in_place_portable(uint32_t cv[8],
                                       const uint8_t block[BLAKE3_BLOCK_LEN],
                                       uint8_t block_len, uint64_t counter,
                                       uint8_t flags) {
  uint32_t state[16];
  compress_pre(state, cv, block, block_len, counter, flags);
  cv[0] = state[0] ^ state[8];
  cv[1] = state[1] ^ state[9];
  cv[2] = state[2] ^ state[10];
  cv[3] = state[3] ^ state[11];
  cv[4] = state[4] ^ state[12];
  cv[5] = state[5] ^ state[13];
  cv[6] = state[6] ^ state[14];
  cv[7] = state[7] ^ state[15];
}

void blake3_compress_xof_portable(const uint32_t cv[8],
                                  const uint8_t block[BLAKE3_BLOCK_LEN],
                                  uint8_t block_len, uint64_t counter,
                                  uint8_t flags, uint8_t out[64]) {
  uint32_t state[16];
  compress_pre(state, cv, block, block_len, counter, flags);

  store32(&out[0 * 4], state[0] ^ state[8]);
  store32(&out[1 * 4], state[1] ^ state[9]);
  store32(&out[2 * 4], state[2] ^ state[10]);
  store32(&out[3 * 4], state[3] ^ state[11]);
  store32(&out[4 * 4], state[4] ^ state[12]);
  store32(&out[5 * 4], state[5] ^ state[13]);
  store32(&out[6 * 4], state[6] ^ state[14]);
  store32(&out[7 * 4], state[7] ^ state[15]);
  store32(&out[8 * 4], state[8] ^ cv[0]);
  store32(&out[9 * 4], state[9] ^ cv[1]);
  store32(&out[10 * 4], state[10] ^ cv[2]);
  store32(&out[11 * 4], state[11] ^ cv[3]);
  store32(&out[12 * 4], state[12] ^ cv[4]);
  store32(&out[13 * 4], state[13] ^ cv[5]);
  store32(&out[14 * 4], state[14] ^ cv[6]);
  store32(&out[15 * 4], state[15] ^ cv[7]);
}

INLINE void hash_one_portable(const uint8_t *input, size_t blocks,
                              const uint32_t key[8], uint64_t counter,
                              uint8_t flags, uint8_t flags_start,
                              uint8_t flags_end, uint8_t out[BLAKE3_OUT_LEN]) {
  uint32_t cv[8];
  memcpy(cv, key, BLAKE3_KEY_LEN);
  uint8_t block_flags = flags | flags_start;
  while (blocks > 0) {
    if (blocks == 1) {
      block_flags |= flags_end;
    }
    blake3_compress_in_place_portable(cv, input, BLAKE3_BLOCK_LEN, counter,
                                      block_flags);
    input = &input[BLAKE3_BLOCK_LEN];
    blocks -= 1;
    block_flags = flags;
  }
  memcpy(out, cv, 32);
}

void blake3_hash_many_portable(const uint8_t *const *inputs, size_t num_inputs,
                               size_t blocks, const uint32_t key[8],
                               uint64_t counter, bool increment_counter,
                               uint8_t flags, uint8_t flags_start,
                               uint8_t flags_end, uint8_t *out) {
  while (num_inputs > 0) {
    hash_one_portable(inputs[0], blocks, key, counter, flags, flags_start,
                      flags_end, out);
    if (increment_counter) {
      counter += 1;
    }
    inputs += 1;
    num_inputs -= 1;
    out = &out[BLAKE3_OUT_LEN];
  }
}

void blake3_hasher_update_portable(blake3_hasher *self, const void *input,
		size_t input_len) {
	const uint8_t *input_bytes = (const uint8_t *)input;

	// If we already have a partial chunk, or if this is the very first chunk
	// (and it could be the root), we need to add bytes to the chunk state.
	bool is_first_chunk = self->chunk.chunk_counter == 0;
	bool maybe_root = is_first_chunk && input_len == BLAKE3_CHUNK_LEN;
	if (maybe_root || chunk_state_len(&self->chunk) > 0) {
		size_t take = BLAKE3_CHUNK_LEN - chunk_state_len(&self->chunk);
		if (take > input_len) {
			take = input_len;
		}
		chunk_state_update(&self->chunk, input_bytes, take);
		input_bytes += take;
		input_len -= take;
		// If we've filled the current chunk and there's more coming, finalize this
		// chunk and proceed. In this case we know it's not the root.
		if (input_len > 0) {
			output_t output = chunk_state_output(&self->chunk);
			uint8_t chunk_cv[32];
			output_chaining_value(&output, chunk_cv);
			hasher_push_chunk_cv(self, chunk_cv, self->chunk.chunk_counter);
			chunk_state_reset(&self->chunk, self->key, self->chunk.chunk_counter + 1);
		} else {
			return;
		}
	}

	// Hash as many whole chunks as we can, without buffering anything. At this
	// point we know none of them can be the root.
	uint8_t out[BLAKE3_OUT_LEN * BLAKE3_MAX_SIMD_DEGREE];
	const uint8_t *chunks[BLAKE3_MAX_SIMD_DEGREE];
	size_t num_chunks = 0;
	while (input_len >= BLAKE3_CHUNK_LEN) {
		size_t chunk_index;

		while (input_len >= BLAKE3_CHUNK_LEN &&
				num_chunks < BLAKE3_MAX_SIMD_DEGREE) {
			chunks[num_chunks] = input_bytes;
			input_bytes += BLAKE3_CHUNK_LEN;
			input_len -= BLAKE3_CHUNK_LEN;
			num_chunks += 1;
		}
		blake3_hash_many_portable(chunks, num_chunks, BLAKE3_CHUNK_LEN / BLAKE3_BLOCK_LEN,
				self->key, self->chunk.chunk_counter, true, self->chunk.flags,
				CHUNK_START, CHUNK_END, out);
		for (chunk_index = 0; chunk_index < num_chunks; chunk_index++) {
			// The chunk state is empty here, but it stores the counter of the next
			// chunk hash we need to push. Use that counter, and then move it forward.
			hasher_push_chunk_cv(self, &out[chunk_index * BLAKE3_OUT_LEN],
					self->chunk.chunk_counter);
			self->chunk.chunk_counter += 1;
		}
		num_chunks = 0;
	}

	// If there's any remaining input less than a full chunk, add it to the chunk
	// state. In that case, also do a final merge loop to make sure the subtree
	// stack doesn't contain any unmerged pairs. The remaining input means we
	// know these merges are non-root. This merge loop isn't strictly necessary
	// here, because hasher_push_chunk_cv already does its own merge loop, but it
	// simplifies blake3_hasher_finalize below.
	if (input_len > 0) {
		while (hasher_needs_merge(self, self->chunk.chunk_counter)) {
			hasher_merge_parent(self);
		}
		chunk_state_update(&self->chunk, input_bytes, input_len);
	}
}

