CSTD := -std=gnu90
CC := gcc
SUBST_CFLAGS = -g -O1 -Wall -D_FORTIFY_SOURCE=2
CFLAGS = $(SUBST_CFLAGS) \
	 $(CSTD) \
	 -D_XOPEN_SOURCE=700  \
	 -fno-strict-aliasing \
	 -fPIC \
	 $(EXTRA_CFLAGS)

LIBBTRFSUTIL_CFLAGS = $(SUBST_CFLAGS) \
		      $(CSTD) \
		      -D_GNU_SOURCE \
		      -fPIC \
		      -fvisibility=hidden \
		      -I$(TOPDIR)/libbtrfsutil \
		      $(EXTRAWARN_CFLAGS) \
		      $(DEBUG_CFLAGS_INTERNAL) \
		      $(EXTRA_CFLAGS)

LDFLAGS = $(SUBST_LDFLAGS) \
	  -rdynamic \
	  $(EXTRA_LDFLAGS)

objects = crc32c.o \
	  hash.o \
	  xxhash.o \
	  blake3.o \
	  blake3_dispatch.o \
	  blake3_portable.o

# The sneves/blake2-avx2 implementation includes a few different message
# loading algorithms, for benchmarking across different compilers and
# microarchitectures. On modern Intel chips (Skylake etc.), the
# PERMUTE_WITH_SHUFFLES usually performs best, and we enable that here with a
# preprocessor flag. The Rust crates in oconnor663/blake2_simd, which were
# originally ported from this C implementation, use the same approach.
BLAKE2B_FLAGS = -DPERMUTE_WITH_SHUFFLES -mavx2

# The BLAKE3 build includes SSE4.1, AVX2, and AVX-512 support by default, with
# runtime CPU feature detection. But there are preprocessor flags available to
# disable each instruction set. We expose them here as environment variables.
# For example, to disable all the SIMD implementations, run:
#     make clean
#     BLAKE3_NO_SSE41=1 BLAKE3_NO_AVX2=1 BLAKE3_NO_AVX512=1 make
BLAKE3_ASM_FILES =
BLAKE3_FLAGS =
ifdef BLAKE3_NO_SSE41
BLAKE3_FLAGS += -DBLAKE3_NO_SSE41
else
BLAKE3_ASM_FILES += blake3_sse41_x86-64_unix.S
endif
ifdef BLAKE3_NO_AVX2
BLAKE3_FLAGS += -DBLAKE3_NO_AVX2
else
BLAKE3_ASM_FILES += blake3_avx2_x86-64_unix.S
endif
ifdef BLAKE3_NO_AVX512
BLAKE3_FLAGS += -DBLAKE3_NO_AVX512
else
BLAKE3_ASM_FILES += blake3_avx512_x86-64_unix.S
endif

.PHONY: all clean
.PHONY: FORCE

all: hash-speedtest

blake2b.o: blake2b.c
	$(CC) $(CFLAGS) $(BLAKE2B_FLAGS) -c $^ -o $@

.c.o:
	$(CC) $(CFLAGS) $(BLAKE3_FLAGS) -c $< -o $@

hash-speedtest: hash-speedtest.c blake2b.o $(objects) $(BLAKE3_ASM_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f -- hash-speedtest *.o
