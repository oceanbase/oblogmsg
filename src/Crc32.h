// //////////////////////////////////////////////////////////
// Crc32.cpp
// Copyright (c) 2011-2015 Stephan Brumme. All rights reserved.
// Slicing-by-16 contributed by Bulat Ziganshin
// see http://create.stephan-brumme.com/disclaimer.html
//

// g++ -o Crc32 Crc32.cpp -O3 -lrt -march=native -mtune=native

// if running on an embedded system, you might consider shrinking the
// big Crc32Lookup table:
// - crc32_bitwise doesn't need it at all
// - crc32_halfbyte has its own small lookup table
// - crc32_1byte    needs only Crc32Lookup[0]
// - crc32_4bytes   needs only Crc32Lookup[0..3]
// - crc32_8bytes   needs only Crc32Lookup[0..7]
// - crc32_4x8bytes needs only Crc32Lookup[0..7]
// - crc32_16bytes  needs all of Crc32Lookup
uint32_t crc32_halfbyte(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_4bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_8bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_4x8bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_16bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_16bytes_prefetch(
    const void* data, size_t length, uint32_t previousCrc32 = 0, size_t prefetchAhead = 256);
#define crc32_fast crc32_16bytes
