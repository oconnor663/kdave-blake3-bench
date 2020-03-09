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
	  blake2b-ref.o \
	  blake3.o \
	  blake3_dispatch.o \
	  blake3_portable.o \
	  blake3_avx2.o \
	  blake3_sse41.o

.PHONY: all clean
.PHONY: FORCE

all: hash-speedtest

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

hash-speedtest: hash-speedtest.c $(objects)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f -- hash-speedtest *.o
