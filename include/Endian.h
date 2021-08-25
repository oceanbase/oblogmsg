#ifndef ENDIANNESS_H
#define ENDIANNESS_H

namespace oceanbase {
namespace logmessage {

/* This should catch all modern GCCs and Clang */
#if (defined __BYTE_ORDER__) && (defined __ORDER_LITTLE_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ENDIANNESS_LE 1
#define ENDIANNESS_BE 0
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ENDIANNESS_LE 0
#define ENDIANNESS_BE 1
#endif
/* Try to derive from arch/compiler-specific macros */
#elif defined(_X86_) || defined(__x86_64__) || defined(__i386__) || defined(__i486__) || defined(__i586__) ||     \
    defined(__i686__) || defined(__MIPSEL) || defined(_MIPSEL) || defined(MIPSEL) || defined(__ARMEL__) ||        \
    (defined(__LITTLE_ENDIAN__) && __LITTLE_ENDIAN__ == 1) || (defined(_LITTLE_ENDIAN) && _LITTLE_ENDIAN == 1) || \
    defined(_M_IX86) || defined(_M_AMD64) /* MSVC */
#define ENDIANNESS_LE 1
#define ENDIANNESS_BE 0
#elif defined(__MIPSEB) || defined(_MIPSEB) || defined(MIPSEB) || defined(__MICROBLAZEEB__) || defined(__ARMEB__) || \
    (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__ == 1) || (defined(_BIG_ENDIAN) && _BIG_ENDIAN == 1)
#define ENDIANNESS_LE 0
#define ENDIANNESS_BE 1
/* Try to get it from a header */
#else
#if defined(__linux)
#include <endian.h>
#else
#include <machine/endian.h>
#endif
#endif

#ifndef ENDIANNESS_LE
#undef ENDIANNESS_BE
#if defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ENDIANNESS_LE 1
#define ENDIANNESS_BE 0
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ENDIANNESS_LE 0
#define ENDIANNESS_BE 1
#endif
#elif defined(BYTE_ORDER) && defined(LITTLE_ENDIAN)
#if BYTE_ORDER == LITTLE_ENDIAN
#define ENDIANNESS_LE 1
#define ENDIANNESS_BE 0
#elif BYTE_ORDER == BIG_ENDIAN
#define ENDIANNESS_LE 0
#define ENDIANNESS_BE 1
#endif
#endif
#endif

/* In case the user passed one of -DENDIANNESS_LE or BE in CPPFLAS,
 *    set the second one too */
#if defined(ENDIANNESS_LE) && !(defined(ENDIANNESS_BE))
#if ENDIANNESS_LE == 0
#define ENDIANNESS_BE 1
#else
#define ENDIANNESS_BE 0
#endif
#elif defined(ENDIANNESS_BE) && !(defined(ENDIANNESS_LE))
#if ENDIANNESS_BE == 0
#define ENDIANNESS_LE 1
#else
#define ENDIANNESS_LE 0
#endif
#endif

#if !(defined(ENDIANNESS_LE)) && !(defined(ENDIANNESS_PORTABLE_CONVERSION))
#error \
    "Sorry, we couldn't detect endiannes for your system! Please set -DENDIANNESS_LE=1 or 0 using your CPPFLAGS/CFLAGS!"
#endif

/*little endian set and get*/
static inline uint8_t swap8(uint8_t __x)
{
  return __x;
}

static inline uint16_t swap16(uint16_t __x)
{
  return (__x << 8) | (__x >> 8);
}

static inline uint32_t swap32(uint32_t __x)
{
  return (__x >> 24) | (__x >> 8 & 0xff00) | (__x << 8 & 0xff0000) | (__x << 24);
}

static inline uint64_t swap64(uint64_t __x)
{
  return ((swap32((uint32_t)__x) + 0ULL) << 32) | (swap32(__x >> 32));
}

static inline void toLeEndian(void* buf, int size)
{
#if ENDIANNESS_LE
  return;
#endif
  switch (size) {
    case 1:
      *((uint8_t*)buf) = swap8(*(uint8_t*)buf);
      break;
    case 2:
      *((uint16_t*)buf) = swap16(*(uint16_t*)buf);
      break;
    case 4:
      *((uint32_t*)buf) = swap32(*(uint32_t*)buf);
      break;
    case 8:
      *((uint64_t*)buf) = swap64(*(uint64_t*)buf);
      break;
  }
}

template <class T>
static inline T toLeEndianByType(T value)
{
#if ENDIANNESS_LE
  return value;
#endif
  toLeEndian(&value, sizeof(T));
  return value;
}

}  // namespace logmessage
}  // namespace oceanbase

#endif
