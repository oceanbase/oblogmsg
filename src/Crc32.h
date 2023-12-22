/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

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
// define endianess and some integer data types
#pragma once

#if defined(_MSC_VER) || defined(__MINGW32__)
typedef unsigned __int8 uint8_t;
typedef unsigned __int32 uint32_t;
typedef signed __int32 int32_t;

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN

#include <xmmintrin.h>
#ifdef __MINGW32__
#define PREFETCH(location) __builtin_prefetch(location)
#else
#define PREFETCH(location) _mm_prefetch(location, _MM_HINT_T0)
#endif
#else
// uint8_t, uint32_t, in32_t
#include <cstdint>
// defines __BYTE_ORDER as __LITTLE_ENDIAN or __BIG_ENDIAN
#include <sys/param.h>

#ifdef __GNUC__
#define PREFETCH(location) __builtin_prefetch(location)
#else
#define PREFETCH(location) ;
#endif
#endif

uint32_t crc32_halfbyte(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_4bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_8bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_4x8bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_16bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
uint32_t crc32_16bytes_prefetch(
    const void* data, size_t length, uint32_t previousCrc32 = 0, size_t prefetchAhead = 256);
#define crc32_fast crc32_16bytes
