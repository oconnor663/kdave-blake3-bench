Benchmark of BLAKE3 used for evaluation as a filesystem checksum, published
[here](https://kdave.github.io/blake3-vs-blake2-in-btrfs/):

|           Hash | Total cycles | Cycles/iteration | Perf vs BLAKE3 | Perf vs BLAKE2b
|           :--- |         ---: |             ---: |           :--- | :---
| BLAKE3  (AVX2) | 111260245256 |            11126 | 1.0            | 0.876 (-13%)
| BLAKE2b (AVX2) | 127009487092 |            12700 | 1.141 (+14%)   | 1.0
| BLAKE2b (AVX)  | 166426785907 |            16642 | 1.496 (+50%)   | 1.310 (+31%)
| BLAKE2b (ref)  | 225053579540 |            22505 | 2.022 (+102%)  | 1.772 (+77%)

Block size is 4KiB, number of iterations: 1000000.

The performance improvement +14% against optimized version of BLAKE2b is smaller
than expected (above 70%). To understand where this comes from, this repository
hosts sources that can be used for testing and evaluation in other
environments.

For continuity and calibration with older results, the tool also measures empty
loop, plain memcpy and crc32c/xxhash/blake2b.

Reference host:

* Intel(R) Xeon(R) CPU E5-1620 v3 @ 3.50GHz, 16G of memory, AVX2 capable

Build:

    $ make hash-speedtest
    $ hash-speedtest [iterations]

## Round 1

Verify that published results and what's measured by current sources match. The
BLAKE3 sources are from 0.1.1.

By default, 100K iterations are run, 1M is there to see if the overhead makes
any difference. There are 3 targets compiled with various `-O` flags.

Conclusion: published and current numbers match, `-O` flags don't bring
significant improvement.

Reference results with 100.000 iterations:

    $ ./hash-speedtest.O1
    Block size: 4096
    Iterations: 100000

        NULL-NOP: cycles:     49731562, c/i      497
     NULL-MEMCPY: cycles:     66853283, c/i      668
          CRC32C: cycles:    181899845, c/i     1818
          XXHASH: cycles:    137766217, c/i     1377
         BLAKE2b: cycles:   2274532357, c/i    22745
          BLAKE3: cycles:   2400285391, c/i    24002
    BLAKE3-SSE41: cycles:   1167069441, c/i    11670
     BLAKE3-AVX2: cycles:   1167974091, c/i    11679

    $ ./hash-speedtest.O2
    Block size: 4096
    Iterations: 100000

        NULL-NOP: cycles:     38871962, c/i      388
     NULL-MEMCPY: cycles:     60576411, c/i      605
          CRC32C: cycles:    185520410, c/i     1855
          XXHASH: cycles:    140206405, c/i     1402
         BLAKE2b: cycles:   1871446270, c/i    18714
     BLAKE3-port: cycles:   2047875279, c/i    20478
    BLAKE3-SSE41: cycles:   1051016715, c/i    10510
     BLAKE3-AVX2: cycles:   1052129371, c/i    10521

    $ ./hash-speedtest.O3
    Block size: 4096
    Iterations: 100000

        NULL-NOP: cycles:     46861642, c/i      468
     NULL-MEMCPY: cycles:     58843572, c/i      588
          CRC32C: cycles:    183644437, c/i     1836
          XXHASH: cycles:    137361306, c/i     1373
         BLAKE2b: cycles:   1859611165, c/i    18596
     BLAKE3-port: cycles:   2044300312, c/i    20443
    BLAKE3-SSE41: cycles:   1049436859, c/i    10494
     BLAKE3-AVX2: cycles:   1051208264, c/i    10512

Reference results with 1.000.000 iterations:

    $ ./hash-speedtest.O1 1000000
    Block size: 4096
    Iterations: 1000000

        NULL-NOP: cycles:    311069562, c/i      311
     NULL-MEMCPY: cycles:    600739577, c/i      600
          CRC32C: cycles:   1744911994, c/i     1744
          XXHASH: cycles:   1327265526, c/i     1327
         BLAKE2b: cycles:  22500254356, c/i    22500
          BLAKE3: cycles:  23999837826, c/i    23999
    BLAKE3-SSE41: cycles:  11672269851, c/i    11672
     BLAKE3-AVX2: cycles:  11677903076, c/i    11677

    $ ./hash-speedtest.O2 1000000
    Block size: 4096
    Iterations: 1000000

        NULL-NOP: cycles:    318622473, c/i      318
     NULL-MEMCPY: cycles:    585150709, c/i      585
          CRC32C: cycles:   1782021818, c/i     1782
          XXHASH: cycles:   1332179797, c/i     1332
         BLAKE2b: cycles:  18637886508, c/i    18637
     BLAKE3-port: cycles:  20486804111, c/i    20486
    BLAKE3-SSE41: cycles:  10511936730, c/i    10511
     BLAKE3-AVX2: cycles:  10516193170, c/i    10516

    $ ./hash-speedtest.O3 1000000
    Block size: 4096
    Iterations: 1000000

        NULL-NOP: cycles:    312371265, c/i      312
     NULL-MEMCPY: cycles:    596678141, c/i      596
          CRC32C: cycles:   1796960915, c/i     1796
          XXHASH: cycles:   1341290405, c/i     1341
         BLAKE2b: cycles:  18554328509, c/i    18554
     BLAKE3-port: cycles:  20471714973, c/i    20471
    BLAKE3-SSE41: cycles:  10498995789, c/i    10498
     BLAKE3-AVX2: cycles:  10504252272, c/i    10504

Sources:

* [BLAKE3 sources](https://github.com/BLAKE3-team/BLAKE3)
* [first benchmarking round](https://kdave.github.io/blake3-vs-blake2-in-btrfs/)
