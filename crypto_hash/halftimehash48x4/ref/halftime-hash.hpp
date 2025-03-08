#pragma once

#include <cassert>
#include <cinttypes>
#include <climits>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <type_traits>

#if (defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L)) || defined(__cpp_if_constexpr)
#  define CONSTEXPR_IF(Z) if constexpr (Z)
#else
#  define CONSTEXPR_IF(Z) if (Z)
#endif

#if defined(__x86_64) || defined(_M_X64)
#  include <immintrin.h>
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
#  include <arm_neon.h>
#endif

namespace halftime_hash {
namespace advanced {
namespace {

inline uint64_t LeftShift(uint64_t a, int s) { return a << s; }
inline uint64_t Minus(uint64_t a, uint64_t b) { return a - b; }
inline uint64_t Plus(uint64_t a, uint64_t b) { return a + b; }
inline uint64_t RightShift32(uint64_t a) { return a >> 32; }
inline uint64_t Sum(uint64_t a) { return a; }
inline uint64_t Xor(uint64_t a, uint64_t b) { return a ^ b; }

inline uint64_t Plus32(uint64_t a, uint64_t b) {
  uint64_t result;
  uint32_t temp[2] = {(uint32_t)a + (uint32_t)b,
                      (uint32_t)(a >> 32) + (uint32_t)(b >> 32)};

  result = temp[0] + (((uint64_t)temp[1]) << 32);
  return result;
}

inline uint64_t Times(uint64_t a, uint64_t b) {
  constexpr uint64_t mask = (((uint64_t)1) << 32) - 1;
  return (a & mask) * (b & mask);
}

struct BlockWrapperScalar {
  using Block = uint64_t;

  static uint64_t LoadBlock(const void *x) {
    Block result;
    memcpy(&result, x, sizeof(result));
    return result;
  }

  static uint64_t LoadBlockNative(const uint64_t *x) { return *x; }
  static uint64_t LoadOne(uint64_t entropy) { return entropy; }
};

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)

using u128 = uint64x2_t;

inline u128 LeftShift(u128 a, int i) { return vshlq_u64(a, vdupq_n_s64(i)); }
inline u128 Minus(u128 a, u128 b) { return vsubq_u64(a, b); }
inline u128 Plus(u128 a, u128 b) { return vaddq_u64(a, b); }
inline u128 Plus32(u128 a, u128 b) {
  return vreinterpretq_u64_u32(
      vaddq_u32(vreinterpretq_u32_u64(a), vreinterpretq_u32_u64(b)));
}
inline u128 RightShift32(u128 a) { return vshrq_n_u64(a, 32); }
inline uint64_t Sum(u128 a) { return vgetq_lane_u64(a, 0) + vgetq_lane_u64(a, 1); }
inline u128 Xor(u128 a, u128 b) { return veorq_u64(a, b); }

inline u128 Times(u128 a, u128 b) {
  uint32x2_t a_lo = vmovn_u64(a);
  uint32x2_t b_lo = vmovn_u64(b);
  return vmull_u32(a_lo, b_lo);
}

struct BlockWrapper128 {
  using Block = u128;

  static u128 LoadBlock(const void *x) {
    auto z = reinterpret_cast<const uint64_t *>(x);
    return vld1q_u64(z);
  }

  static u128 LoadBlockNative(const uint64_t *x) { return vld1q_u64(x); }

  static u128 LoadOne(uint64_t entropy) { return vdupq_n_u64(entropy); }
};

#elif defined(__SSE2__)

using u128 = __m128i;

inline u128 LeftShift(u128 a, int i) { return _mm_slli_epi64(a, i); }
inline u128 Minus(u128 a, u128 b) { return _mm_sub_epi64(a, b); }
inline u128 Plus(u128 a, u128 b) { return _mm_add_epi64(a, b); }
inline u128 Plus32(u128 a, u128 b) { return _mm_add_epi32(a, b); }
inline u128 RightShift32(u128 a) { return _mm_srli_epi64(a, 32); }
// _mm_extract_epi64 assumes SSE4.1 is also available
inline uint64_t Sum(u128 a) { return _mm_cvtsi128_si64(a) + _mm_extract_epi64(a, 1); }
inline u128 Times(u128 a, u128 b) { return _mm_mul_epu32(a, b); }
inline u128 Xor(u128 a, u128 b) { return _mm_xor_si128(a, b); }

struct BlockWrapper128 {
  using Block = u128;

  static u128 LoadBlock(const void *x) {
    auto y = reinterpret_cast<const u128 *>(x);
    return _mm_loadu_si128(y);
  }

  static u128 LoadBlockNative(const uint64_t *x) {
    auto y = reinterpret_cast<const u128 *>(x);
    return _mm_loadu_si128(y);
  }

  static u128 LoadOne(uint64_t entropy) { return _mm_set1_epi64x(entropy); }
};

#endif  // ARM NEON and SSE2

#if defined(__AVX2__)

using u256 = __m256i;

inline u256 LeftShift(u256 a, int i) { return _mm256_slli_epi64(a, i); }
inline u256 Minus(u256 a, u256 b) { return _mm256_sub_epi64(a, b); }
inline u256 Plus(u256 a, u256 b) { return _mm256_add_epi64(a, b); }
inline u256 Plus32(u256 a, u256 b) { return _mm256_add_epi32(a, b); }
inline u256 RightShift32(u256 a) { return _mm256_srli_epi64(a, 32); }
inline u256 Times(u256 a, u256 b) { return _mm256_mul_epu32(a, b); }
inline u256 Xor(u256 a, u256 b) { return _mm256_xor_si256(a, b); }

inline uint64_t Sum(u256 a) {
  auto c = _mm256_extracti128_si256(a, 0);
  auto d = _mm256_extracti128_si256(a, 1);

  c = _mm_add_epi64(c, d);
#  ifndef _MSC_VER
  static_assert(sizeof(c[0]) == sizeof(uint64_t), "u256 too granular");
  static_assert(sizeof(c) == 2 * sizeof(uint64_t), "u256 too granular");
#  endif
  // _mm_extract_epi64 assumes SSE4.1 is also available (should be always present when
  // AVX2 is enabled)
  return _mm_cvtsi128_si64(c) + _mm_extract_epi64(c, 1);
}

struct BlockWrapper256 {
  using Block = u256;

  static u256 LoadBlock(const void *x) {
    auto y = reinterpret_cast<const u256 *>(x);
    return _mm256_loadu_si256(y);
  }

  static u256 LoadBlockNative(const uint64_t *x) {
    auto y = reinterpret_cast<const u256 *>(x);
    return _mm256_loadu_si256(y);
  }

  static u256 LoadOne(uint64_t entropy) { return _mm256_set1_epi64x(entropy); }
};

#endif  // AVX2

#if defined(__AVX512F__)

using u512 = __m512i;

inline u512 LeftShift(u512 a, int i) { return _mm512_slli_epi64(a, i); }
inline u512 Minus(u512 a, u512 b) { return _mm512_sub_epi64(a, b); }
inline u512 Plus(u512 a, u512 b) { return _mm512_add_epi64(a, b); }
inline u512 Plus32(u512 a, u512 b) { return _mm512_add_epi32(a, b); }
// Alternate RightShift32: _mm512_shuffle_epi32(a, _MM_PERM_ACAC)
inline u512 RightShift32(u512 a) { return _mm512_srli_epi64(a, 32); }
inline u512 Times(u512 a, u512 b) { return _mm512_mul_epu32(a, b); }
inline u512 Xor(u512 a, u512 b) { return _mm512_xor_epi32(a, b); }
inline uint64_t Sum(u512 a) { return _mm512_reduce_add_epi64(a); }

struct BlockWrapper512 {
  using Block = u512;

  static Block LoadBlock(const void *x) { return _mm512_loadu_si512(x); }
  static Block LoadBlockNative(const uint64_t *x) { return _mm512_loadu_si512(x); }
  static Block LoadOne(uint64_t entropy) { return _mm512_set1_epi64(entropy); }
};

#endif  // AVX-512

template <typename T>
T MultiplyAdd(const T &summand, const T &factor1, const T &factor2) {
  return Plus(summand, Times(factor1, factor2));
}

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)

u128 MultiplyAdd(const u128 &summand, const u128 &factor1, const u128 &factor2) {
  return vmlal_u32(summand, vmovn_u64(factor1), vmovn_u64(factor2));
}

#endif

// BestSimd<log_block_width>::SimdDispatch is one of the BlockWrappers - the widest one
// available for operating on 1 << (log_block_width - 1) lanes at once. For instance, in
// AVX2 registers, we have four 64-bit lanes, so BestSimd<4>::SimdDispatch is
// BlockWrapper256. Note that this is also true for BestSimd<3>::SimdDispatch. For the
// former, of course, to operate on 1<<(4-1) lanes at once, we will have to use two
// 256-bit registers. This is what Repeat* is for, below.
template <int log_block_width>
struct BestSimd {};

#define BEST_SPECIALIZE(LOG_BLOCK_WIDTH, WRAPPER) \
  template <>                                     \
  struct BestSimd<LOG_BLOCK_WIDTH> {              \
    using SimdDispatch = WRAPPER;                 \
  };

#if defined(__AVX512F__)
#  define HALFTIME_IMPL_STR "avx512f"
#  define HALFTIME_IMPL_WIDEST_SIMD_LOG_BLOCK_WIDTH 4
#elif defined(__AVX2__)
#  define HALFTIME_IMPL_STR "avx2"
#  define HALFTIME_IMPL_WIDEST_SIMD_LOG_BLOCK_WIDTH 3
#elif defined(__SSE2__)
#  define HALFTIME_IMPL_STR "sse2"
#  define HALFTIME_IMPL_WIDEST_SIMD_LOG_BLOCK_WIDTH 2
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
#  define HALFTIME_IMPL_STR "neon"
#  define HALFTIME_IMPL_WIDEST_SIMD_LOG_BLOCK_WIDTH 2
#else
#  define HALFTIME_IMPL_STR "portable"
#  define HALFTIME_IMPL_WIDEST_SIMD_LOG_BLOCK_WIDTH 1
#endif

#if defined(__AVX512F__)
BEST_SPECIALIZE(4, BlockWrapper512);
#elif defined(__AVX2__)
BEST_SPECIALIZE(4, BlockWrapper256);
#elif defined(__SSE2__) || defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
BEST_SPECIALIZE(4, BlockWrapper128);
#else
BEST_SPECIALIZE(4, BlockWrapperScalar);
#endif

#if defined(__AVX2__)
BEST_SPECIALIZE(3, BlockWrapper256);
#elif defined(__SSE2__ || defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
BEST_SPECIALIZE(3, BlockWrapper128);
#else
BEST_SPECIALIZE(3, BlockWrapperScalar);
#endif

#if defined(__SSE2__) || defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
BEST_SPECIALIZE(2, BlockWrapper128);
#else
BEST_SPECIALIZE(2, BlockWrapperScalar);
#endif

BEST_SPECIALIZE(1, BlockWrapperScalar);

constexpr int MessageLength(int hh_output_length) {
  return (hh_output_length == 1)
             ? 12
             : ((hh_output_length == 2)
                    ? 6
                    : ((hh_output_length == 3) ? 7
                                               : ((hh_output_length == 4)   ? 7
                                                  : (hh_output_length == 5) ? 5
                                                                            : 4)));
}

constexpr int BlockLength(int hh_output_length) {
  return hh_output_length + MessageLength(hh_output_length) - 1;
}

// Must be 3 for Encode 3, 4, and 5 due to the error correction codes used, though
// divisible by three would also work with some changes below.
constexpr int Log2Alphabet(int hh_output_length) {
  return 32 / BlockLength(hh_output_length);
}

template <typename Block>
inline void Encode2(Block raw_io[BlockLength(2) * Log2Alphabet(2)]) {
  static constexpr int hh_output_length = 2;
  static constexpr int log2_alphabet = Log2Alphabet(2);
  auto io = reinterpret_cast<Block(*)[log2_alphabet]>(raw_io);

  for (int i = 0; i < log2_alphabet; ++i) {
    io[BlockLength(hh_output_length) - 1][i] = io[0][i];
    for (int j = 1; j < MessageLength(hh_output_length); ++j) {
      io[BlockLength(hh_output_length) - 1][i] =
          Xor(io[BlockLength(hh_output_length) - 1][i], io[j][i]);
    }
  }
}

// https://docs.switzernet.com/people/emin-gabrielyan/051101-erasure-9-7-resilient/
template <typename Block>
inline void Encode3(Block raw_io[BlockLength(3) * 3]) {
  static constexpr int hh_output_length = 3;
  static constexpr int log2_alphabet = 3;
  static_assert(log2_alphabet == Log2Alphabet(hh_output_length),
                "Alphabet not expected size");
  auto io = reinterpret_cast<Block(*)[log2_alphabet]>(raw_io);
  constexpr unsigned x = 0, y = 1, z = 2;

  const Block *iter = io[0];

  io[7][x] = io[8][x] = iter[x];
  io[7][y] = io[8][y] = iter[y];
  io[7][z] = io[8][z] = iter[z];
  iter += 1;

  // DistributeRaw and Distribute3 are generic, but when lifted out of
  // function scope, the code slows down due to unknown compiler code
  // optimization limitations.
  auto DistributeRaw = [io, &iter](unsigned slot, unsigned label,
                                   std::initializer_list<unsigned> rest) {
    for (unsigned i : rest) {
      io[slot][i] = Xor(io[slot][i], iter[label]);
    }
  };

  auto Distribute3 = [&iter, DistributeRaw, x, y, z](unsigned idx,
                                                     std::initializer_list<unsigned> a,
                                                     std::initializer_list<unsigned> b,
                                                     std::initializer_list<unsigned> c) {
    // Need capture for MSVC; need cast for Clang
    static_cast<void>(x);
    static_cast<void>(y);
    static_cast<void>(z);
    DistributeRaw(idx, x, a);
    DistributeRaw(idx, y, b);
    DistributeRaw(idx, z, c);
    iter += 1;
  };

  while (iter != io[MessageLength(hh_output_length)]) {
    Distribute3(7, {x}, {y}, {z});
  }

  iter = io[1];
  // 11,89,120,177,244,266,297
  Distribute3(8, {z}, {x, z}, {y});           // 177
  Distribute3(8, {x, z}, {x, y, z}, {y, z});  // 244
  Distribute3(8, {y}, {y, z}, {x, z});        // 89
  Distribute3(8, {x, y}, {z}, {x});           // 120
  Distribute3(8, {y, z}, {x, y}, {x, y, z});  // 266
  Distribute3(8, {x, y, z}, {x}, {x, y});     //  297
}

// https://docs.switzernet.com/people/emin-gabrielyan/051102-erasure-10-7-resilient/
template <typename Block>
inline void Encode4(Block raw_io[BlockLength(4) * 3]) {
  static constexpr int hh_output_length = 4;
  static constexpr int log2_alphabet = 3;
  static_assert(log2_alphabet == Log2Alphabet(hh_output_length),
                "Alphabet not expected size");

  auto io = reinterpret_cast<Block(*)[log2_alphabet]>(raw_io);

  constexpr unsigned x = 0, y = 1, z = 2;

  const Block *iter = io[0];

  io[7][x] = io[8][x] = io[9][x] = iter[x];
  io[7][y] = io[8][y] = io[9][y] = iter[y];
  io[7][z] = io[8][z] = io[9][z] = iter[z];
  iter += 1;

  auto DistributeRaw = [io, &iter](unsigned slot, unsigned label,
                                   std::initializer_list<unsigned> rest) {
    for (unsigned i : rest) {
      io[slot][i] = Xor(io[slot][i], iter[label]);
    }
  };

  auto Distribute3 = [&iter, DistributeRaw, x, y, z](unsigned idx,
                                                     std::initializer_list<unsigned> a,
                                                     std::initializer_list<unsigned> b,
                                                     std::initializer_list<unsigned> c) {
    // Need capture for MSVC; need cast for Clang
    static_cast<void>(x);
    static_cast<void>(y);
    static_cast<void>(z);
    DistributeRaw(idx, x, a);
    DistributeRaw(idx, y, b);
    DistributeRaw(idx, z, c);
    iter += 1;
  };

  while (iter != io[MessageLength(hh_output_length)]) {
    Distribute3(7, {x}, {y}, {z});
  }

  iter = io[1];
  Distribute3(8, {z}, {x, z}, {y});           // 73
  Distribute3(8, {x, z}, {x, y, z}, {y, z});  // 140
  Distribute3(8, {y}, {y, z}, {x, z});        // 167
  Distribute3(8, {x, y}, {z}, {x});           // 198
  Distribute3(8, {y, z}, {x, y}, {x, y, z});  // 292
  Distribute3(8, {x, y, z}, {x}, {x, y});     // 323

  iter = io[1];
  Distribute3(9, {x, z}, {x, y, z}, {y, z});  // 140
  Distribute3(9, {x, y}, {z}, {x});           // 198
  Distribute3(9, {z}, {x, z}, {y});           // 73
  Distribute3(9, {y, z}, {x, y}, {x, y, z});  // 292
  Distribute3(9, {x, y, z}, {x}, {x, y});     // 323
  Distribute3(9, {y}, {y, z}, {x, z});        // 167
}

// [[11,11,11,11,11], [11,140,198,73,167], [181,39,257,94,316], [155,263,121,244,54]]
template <typename Block>
inline void Encode5(Block raw_io[BlockLength(5) * 3]) {
  static constexpr int hh_output_length = 5;
  static constexpr int log2_alphabet = 3;
  static_assert(log2_alphabet == Log2Alphabet(hh_output_length),
                "Alphabet not expected size");

  auto io = reinterpret_cast<Block(*)[log2_alphabet]>(raw_io);

  constexpr unsigned x = 0, y = 1, z = 2;

  const Block *iter = io[0];

  io[5][x] = io[6][x] = iter[x];
  io[5][y] = io[6][y] = iter[y];
  io[5][z] = io[6][z] = iter[z];

  // 181
  io[7][x] = iter[z];
  io[7][y] = Xor(iter[x], iter[z]);
  io[7][z] = Xor(iter[y], iter[z]);

  // 155
  io[8][x] = iter[z];
  io[8][y] = iter[y];
  io[8][z] = iter[x];

  iter += 1;

  auto DistributeRaw = [io, &iter](unsigned slot, unsigned label,
                                   std::initializer_list<unsigned> rest) {
    for (unsigned i : rest) {
      io[slot][i] = Xor(io[slot][i], iter[label]);
    }
  };

  auto Distribute3 = [DistributeRaw, x, y, z](unsigned idx,
                                              std::initializer_list<unsigned> a,
                                              std::initializer_list<unsigned> b,
                                              std::initializer_list<unsigned> c) {
    // Need capture for MSVC; need cast for Clang
    static_cast<void>(x);
    static_cast<void>(y);
    static_cast<void>(z);
    DistributeRaw(idx, x, a);
    DistributeRaw(idx, y, b);
    DistributeRaw(idx, z, c);
  };

  // [[11,11,11,11,11], [11,140,198,73,167], [181,39,257,94,316], [155,263,121,244,54]]

  iter = io[1];                               // [11,11,181,155]
  Distribute3(5, {x}, {y}, {z});              // 11
  Distribute3(6, {x, y}, {y, z}, {x, y, z});  // 140
  Distribute3(7, {x}, {y, z}, {z});           // 39
  Distribute3(8, {y, z}, {x, y}, {z});        // 263
  ++iter;

  Distribute3(5, {x}, {y}, {z});        // 11
  Distribute3(6, {x, z}, {x}, {y});     // 198
  Distribute3(7, {y, z}, {y}, {x, z});  // 257
  Distribute3(8, {x, y}, {z}, {y});     // 121
  ++iter;

  Distribute3(5, {x}, {y}, {z});           // 11
  Distribute3(6, {y}, {z}, {x, y});        // 73
  Distribute3(7, {y}, {x, y, z}, {x, y});  // 94
  Distribute3(8, {y}, {x}, {x, z});        // 54
  ++iter;

  Distribute3(5, {x}, {y}, {z});              // 11
  Distribute3(6, {z}, {x, y}, {y, z});        // 167
  Distribute3(7, {x, y, z}, {z}, {x});        // 316
  Distribute3(8, {x, z}, {x, y, z}, {y, z});  // 244
}

/*
Dual code:

xyz 0 0 0 0 xyz xyz       z,xz,yz  zyx
0 xyz 0 0 0 xyz xy,yz,xyz x,yz,z   yz,xy,z
0 0 xyz 0 0 xyz xz,x,y    yz,y,xz  xy,z,y
0 0 0 xyz 0 xyz y,z,xy    y,xyz,xy y,x,xz
0 0 0 0 xyz xyz z,xy,yz   xyz,z,x  xz,xyz,yz

turns into

xyz     xyz       xyz     xyz      xyz       xyz 0 0 0
xyz     xy,yz,xyz xz,x,y  y,z,xy   z,xy,yz   0 xyz 0 0
z,xz,yz x,yz,z    yz,y,xz y,xyz,xy xyz,z,x   0 0 xyz 0
zyx     yz,xy,z   xy,z,y  y,x,xz   xz,xyz,yz 0 0 0 xyz

But rearranging columns is fine:

xyz 0 0 0 xyz     xyz       xyz     xyz      xyz
0 xyz 0 0 xyz     xy,yz,xyz xz,x,y  y,z,xy   z,xy,yz
0 0 xyz 0 z,xz,yz x,yz,z    yz,y,xz y,xyz,xy xyz,z,x
0 0 0 xyz zyx     yz,xy,z   xy,z,y  y,x,xz   xz,xyz,yz

 */
template <typename Block>
inline void Encode6(Block raw_io[BlockLength(6) * 3]) {
  static constexpr int hh_output_length = 6;
  static constexpr int log2_alphabet = 3;
  static_assert(log2_alphabet == Log2Alphabet(hh_output_length),
                "Alphabet not expected size");

  auto io = reinterpret_cast<Block(*)[log2_alphabet]>(raw_io);

  constexpr unsigned x = 0, y = 1, z = 2;

  const Block *iter = io[0];

  for (auto j : {x, y, z}) io[4][j] = io[5][j] = io[6][j] = io[7][j] = io[8][j] = iter[j];

  auto DistributeRaw = [io, &iter](unsigned slot, unsigned label,
                                   std::initializer_list<unsigned> rest) {
    for (unsigned i : rest) {
      io[slot][i] = Xor(io[slot][i], iter[label]);
    }
  };

  auto Distribute3 = [DistributeRaw, x, y, z](unsigned idx,
                                              std::initializer_list<unsigned> a,
                                              std::initializer_list<unsigned> b,
                                              std::initializer_list<unsigned> c) {
    // Need capture for MSVC; need cast for Clang
    static_cast<void>(x);
    static_cast<void>(y);
    static_cast<void>(z);
    DistributeRaw(idx, x, a);
    DistributeRaw(idx, y, b);
    DistributeRaw(idx, z, c);
  };

  iter = io[1];
  Distribute3(4, {x}, {y}, {z});
  Distribute3(5, {x, y}, {y, z}, {x, y, z});
  Distribute3(6, {x, z}, {x}, {y});
  Distribute3(7, {y}, {z}, {x, y});
  Distribute3(8, {z}, {x, y}, {y, z});
  ++iter;

  Distribute3(4, {z}, {x, z}, {y, z});
  Distribute3(5, {x}, {y, z}, {z});
  Distribute3(6, {x, y}, {y}, {x, z});
  Distribute3(7, {x}, {x, y, z}, {x, y});
  Distribute3(8, {x, y, z}, {z}, {x});
  ++iter;

  Distribute3(4, {z}, {y}, {x});
  Distribute3(5, {y, z}, {x, y}, {z});
  Distribute3(6, {x, y}, {z}, {y});
  Distribute3(7, {y}, {x}, {x, z});
  Distribute3(8, {x, z}, {x, y, z}, {y, z});
}

template <typename Badger, typename Block>
inline void Combine1(const Block input[BlockLength(1)], Block output[1]);

template <typename Badger, typename Block>
inline void Combine2(const Block input[BlockLength(2)], Block output[2]);

template <typename Badger, typename Block>
inline void Combine3(const Block input[BlockLength(3)], Block output[3]);

template <typename Badger, typename Block>
inline void Combine4(const Block input[BlockLength(4)], Block output[4]);

template <typename Badger, typename Block>
inline void Combine5(const Block input[BlockLength(5)], Block output[5]);

template <typename Badger, typename Block>
inline void Combine6(const Block input[BlockLength(6)], Block output[6]);

constexpr inline uint32_t FloorLog(uint64_t a, uint64_t b) {
  return (0 == a) ? 0 : ((b < a) ? 0 : (1 + (FloorLog(a, b / a))));
}

template <typename BlockWrapper, unsigned log2_alphabet, unsigned message_length,
          unsigned block_length, unsigned fanout = 8>
struct EhcBadger {
  static_assert(block_length >= message_length, "codes have to code something?");
  static constexpr unsigned hh_output_length = block_length - message_length + 1;
  using Block = typename BlockWrapper::Block;

  static Block Mix(const Block &accum, const Block &input, const Block &entropy) {
    Block output = Plus32(entropy, input);
    Block twin = RightShift32(output);

    output = MultiplyAdd(accum, output, twin);
    return output;
  }

  static Block MixOne(const Block &accum, const Block &input, uint64_t entropy) {
    return Mix(accum, input, BlockWrapper::LoadOne(entropy));
  }

  static Block MixNone(const Block &input, uint64_t entropy_word) {
    Block entropy = BlockWrapper::LoadOne(entropy_word);
    Block output = Plus32(entropy, input);
    Block twin = RightShift32(output);

    output = Times(output, twin);
    return output;
  }

  static void EhcUpperLayer(const Block (&input)[fanout][hh_output_length],
                            const uint64_t entropy[hh_output_length * (fanout - 1)],
                            Block (&output)[hh_output_length]) {
    for (unsigned i = 0; i < hh_output_length; ++i) {
      output[i] = input[0][i];
      for (unsigned j = 1; j < fanout; ++j) {
        output[i] = MixOne(output[i], input[j][i], entropy[(fanout - 1) * i + j - 1]);
      }
    }
  }

  static void Encode(Block (&io)[block_length][log2_alphabet]) {
    static_assert(1 <= hh_output_length && hh_output_length <= 6, "unknown width");
    // Can pass IO as a reference (rather than taking the address of the
    // 0th element) with constexpr if or static dispatch. Right now the
    // compiler MAKES this into static dispatch at codegen/optimization
    // time, but the type checker doesn't know that.
    CONSTEXPR_IF(hh_output_length == 2) { return Encode2<Block>(&io[0][0]); }
    CONSTEXPR_IF(hh_output_length == 3) { return Encode3<Block>(&io[0][0]); }
    CONSTEXPR_IF(hh_output_length == 4) { return Encode4<Block>(&io[0][0]); }
    CONSTEXPR_IF(hh_output_length == 5) { return Encode5<Block>(&io[0][0]); }
    CONSTEXPR_IF(hh_output_length == 6) { return Encode6<Block>(&io[0][0]); }
  }

  static Block SimpleTimes(std::integral_constant<int, 1>, const Block &x) { return x; }

  static Block SimpleTimes(std::integral_constant<int, 2>, const Block &x) {
    return LeftShift(x, 1);
  }

  static Block SimpleTimes(std::integral_constant<int, 3>, const Block &x) {
    return Plus(x, LeftShift(x, 1));
  }

  static Block SimpleTimes(std::integral_constant<int, 4>, const Block &x) {
    return LeftShift(x, 2);
  }

  static Block SimpleTimes(std::integral_constant<int, 5>, const Block &x) {
    return Plus(x, LeftShift(x, 2));
  }

  static Block SimpleTimes(std::integral_constant<int, 6>, const Block &x) {
    return Plus(LeftShift(x, 1), LeftShift(x, 2));
  }

  static Block SimpleTimes(std::integral_constant<int, 7>, const Block &x) {
    return Minus(LeftShift(x, 3), x);
  }

  static Block SimpleTimes(std::integral_constant<int, 8>, const Block &x) {
    return LeftShift(x, 3);
  }

  static Block SimpleTimes(std::integral_constant<int, 9>, const Block &x) {
    return Plus(x, LeftShift(x, 3));
  }

  template <int a>
  static Block SimplerTimes(const Block &x) {
    return SimpleTimes(std::integral_constant<int, a>{}, x);
  }

  template <int a, int b>
  static void Dot2(Block sinks[2], const Block &x) {
    sinks[0] = Plus(sinks[0], SimplerTimes<a>(x));
    sinks[1] = Plus(sinks[1], SimplerTimes<b>(x));
  }

  template <int a, int b, int c>
  static void Dot3(Block sinks[3], const Block &x) {
    Dot2<a, b>(sinks, x);
    sinks[2] = Plus(sinks[2], SimplerTimes<c>(x));
  }

  template <int a, int b, int c, int d>
  static void Dot4(Block sinks[4], const Block &x) {
    Dot3<a, b, c>(sinks, x);
    sinks[3] = Plus(sinks[3], SimplerTimes<d>(x));
  }

  template <int a, int b, int c, int d, int e>
  static void Dot5(Block sinks[5], const Block &x) {
    Dot4<a, b, c, d>(sinks, x);
    sinks[4] = Plus(sinks[4], SimplerTimes<e>(x));
  }

  template <int a, int b, int c, int d, int e, int f>
  static void Dot6(Block sinks[6], const Block &x) {
    Dot5<a, b, c, d, e>(sinks, x);
    sinks[5] = Plus(sinks[5], SimplerTimes<e>(x));
  }

  static void Combine(const Block (&input)[block_length],
                      Block (&output)[hh_output_length]) {
    // Can pass input and output as references with constexpr if or static
    // dispatch. Right now the compiler MAKES this into static dispatch at
    // codegen/optimization time, but the type checker doesn't know that.
    CONSTEXPR_IF(hh_output_length == 1) { return Combine1<EhcBadger>(input, output); }
    CONSTEXPR_IF(hh_output_length == 2) { return Combine2<EhcBadger>(input, output); }
    CONSTEXPR_IF(hh_output_length == 3) { return Combine3<EhcBadger>(input, output); }
    CONSTEXPR_IF(hh_output_length == 4) { return Combine4<EhcBadger>(input, output); }
    CONSTEXPR_IF(hh_output_length == 5) { return Combine5<EhcBadger>(input, output); }
    CONSTEXPR_IF(hh_output_length == 6) { return Combine6<EhcBadger>(input, output); }
  }

  static void Load(
      const unsigned char input[message_length * log2_alphabet * sizeof(Block)],
      Block output[message_length][log2_alphabet]) {
    for (unsigned i = 0; i < message_length; ++i) {
      for (unsigned j = 0; j < log2_alphabet; ++j) {
        output[i][j] =
            BlockWrapper::LoadBlock(&input[(i * log2_alphabet + j) * sizeof(Block)]);
      }
    }
  }

  static void Hash(const Block (&input)[block_length][log2_alphabet],
                   const uint64_t entropy[block_length][log2_alphabet],
                   Block (&output)[block_length]) {
    for (unsigned i = 0; i < block_length; ++i) {
      output[i] = MixNone(input[i][0], entropy[i][0]);
    }
    for (unsigned j = 1; j < log2_alphabet; ++j) {
      for (unsigned i = 0; i < block_length; ++i) {
        output[i] = MixOne(output[i], input[i][j], entropy[i][j]);
      }
    }
  }

  static void EhcBaseLayer(
      const unsigned char input[message_length * log2_alphabet * sizeof(Block)],
      const uint64_t raw_entropy[block_length][log2_alphabet],
      Block (&output)[hh_output_length]) {
    Block scratch[block_length][log2_alphabet];
    Block tmpout[block_length];

    Load(input, scratch);
    Encode(scratch);
    Hash(scratch, raw_entropy, tmpout);
    Combine(tmpout, output);
  }

  static void DfsTreeHash(const unsigned char *data, size_t block_group_length,
                          Block stack[][fanout][hh_output_length], int stack_lengths[],
                          const uint64_t *entropy) {
    auto entropy_matrix = reinterpret_cast<const uint64_t(*)[log2_alphabet]>(entropy);

    for (size_t k = 0; k < block_group_length; ++k) {
      int i = 0;
      while (stack_lengths[i] == fanout) {
        ++i;
      }
      for (int j = i - 1; j >= 0; --j) {
        EhcUpperLayer(stack[j],
                      &entropy[block_length * log2_alphabet /* <- the amount of entropy
                                                          used by EhcBaseLayer() */
                               + (fanout - 1) * hh_output_length * j],
                      stack[j + 1][stack_lengths[j + 1]]);
        stack_lengths[j] = 0;
        stack_lengths[j + 1] += 1;
      }

      EhcBaseLayer(&data[k * message_length * log2_alphabet * sizeof(Block)],
                   entropy_matrix, stack[0][stack_lengths[0]]);
      stack_lengths[0] += 1;
    }
  }

  static constexpr size_t kBlockWords = sizeof(Block) / sizeof(uint64_t);

  static constexpr uint32_t Height(size_t word_length) {
    return FloorLog(fanout, word_length / (kBlockWords * message_length * log2_alphabet));
  }

  static constexpr uint32_t GetEntropyBytesNeeded(size_t n) {
    return block_length * log2_alphabet * sizeof(Block) +
           Height(n) * hh_output_length * (fanout - 1) * sizeof(uint64_t);
  }

  struct BlockGreedy {
   private:
    const uint64_t *seeds;
    Block accum[hh_output_length] = {};

   public:
    BlockGreedy(const uint64_t seeds[]) : seeds(seeds) {}

    void Insert(const Block (&x)[hh_output_length]) {
      for (unsigned i = 0; i < hh_output_length; ++i) {
        accum[i] = Mix(accum[i], x[i], BlockWrapper::LoadBlockNative(seeds));
        seeds += sizeof(Block) / sizeof(uint64_t);
      }
    }

    void Insert(const Block &x) {
      for (unsigned i = 0; i < hh_output_length; ++i) {
        accum[i] = Mix(
            accum[i], x,
            BlockWrapper::LoadBlockNative(&seeds[i * sizeof(Block) / sizeof(uint64_t)]));
      }
      // Toeplitz
      seeds += sizeof(Block) / sizeof(uint64_t);
    }

    void Hash(uint64_t output[hh_output_length]) const {
      for (unsigned i = 0; i < hh_output_length; ++i) {
        output[i] = Sum(accum[i]);
      }
    }
  };

  static constexpr unsigned kMaxStackSize =
      Height(~0ull / (sizeof(Block) / sizeof(uint64_t)));

  static void DfsGreedyFinalizer(const Block stack[][fanout][hh_output_length],
                                 const int stack_lengths[],
                                 const unsigned char *char_input, size_t char_length,
                                 const uint64_t *entropy,
                                 uint64_t output[hh_output_length]) {
    BlockGreedy b(entropy);

    for (unsigned int j = 0; j < kMaxStackSize; ++j) {
      for (int k = 0; k < stack_lengths[j]; k += 1) {
        b.Insert(stack[j][k]);
      }
    }

    size_t i = 0;
    for (; i + sizeof(Block) <= char_length; i += sizeof(Block)) {
      b.Insert(BlockWrapper::LoadBlock(&char_input[i]));
    }

    Block extra = {};
    memcpy(&extra, &char_input[i], char_length - i);
    b.Insert(extra);
    b.Hash(output);
  }
};  // EhcBadger

template <typename Badger, typename Block>
inline void Combine1(const Block input[BlockLength(1)], Block output[1]) {
  output[0] = input[0];
  for (int i = 1; i < BlockLength(1); ++i) {
    output[0] = Xor(output[0], input[i]);
  }
}

template <typename Badger, typename Block>
inline void Combine2(const Block input[BlockLength(2)], Block output[2]) {
  output[0] = input[0];
  output[1] = input[1];

  Badger::template Dot2<1, 1>(output, input[2]);
  Badger::template Dot2<1, 2>(output, input[3]);
  Badger::template Dot2<2, 1>(output, input[4]);
  Badger::template Dot2<1, 4>(output, input[5]);
  Badger::template Dot2<4, 1>(output, input[6]);
}

// evenness: 2 weight: 10
//  0   0   1   4   1   1   2   2   1
//  1   1   0   0   1   4   1   2   2
//  1   4   1   1   0   0   2   1   2

template <typename Badger, typename Block>
inline void Combine3(const Block input[BlockLength(3)], Block output[3]) {
  output[1] = input[0];
  output[2] = input[0];

  output[1] = Plus(output[1], input[1]);
  output[2] = Plus(output[2], LeftShift(input[1], 2));

  output[0] = input[2];
  output[2] = Plus(output[2], input[2]);

  output[0] = Plus(output[0], LeftShift(input[3], 2));
  output[2] = Plus(output[2], input[3]);

  output[0] = Plus(output[0], input[4]);
  output[1] = Plus(output[1], input[4]);

  output[0] = Plus(output[0], input[5]);
  output[1] = Plus(output[1], LeftShift(input[5], 2));

  Badger::template Dot3<2, 1, 2>(output, input[6]);
  Badger::template Dot3<2, 2, 1>(output, input[7]);
  Badger::template Dot3<1, 2, 2>(output, input[8]);
}

// evenness: 4 weight: 16
//   8   8   0   2   1   8   2   1   2   4
//   0   8   1   0   1   1   4   1   4   2
//   1   8   1   4   2   8   1   4   1   2
//   8   1   1   1   1   8   1   8   4   1

// evenness: 3 weight: 21
// 0   0   0   1   1   4   2   4   1   1
// 0   1   2   0   0   1   1   2   4   1
// 2   0   1   0   4   0   1   1   1   1
// 1   1   0   1   0   0   4   1   2   8

template <typename Badger, typename Block>
inline void Combine4(const Block input[BlockLength(4)], Block output[4]) {
  output[2] = LeftShift(input[0], 1);
  output[3] = input[0];

  output[1] = input[1];
  output[3] = Plus(output[3], input[1]);

  output[1] = Plus(output[1], LeftShift(input[2], 1));
  output[2] = Plus(output[2], input[2]);

  output[0] = input[3];
  output[3] = Plus(output[3], input[3]);

  output[0] = Plus(output[0], input[4]);
  output[2] = Plus(output[2], LeftShift(input[4], 2));

  output[0] = Plus(output[0], LeftShift(input[5], 2));
  output[1] = Plus(output[1], input[5]);

  Badger::template Dot4<2, 1, 1, 4>(output, input[6]);
  Badger::template Dot4<4, 2, 1, 1>(output, input[7]);
  Badger::template Dot4<1, 4, 1, 2>(output, input[8]);
  Badger::template Dot4<1, 1, 1, 8>(output, input[9]);
}

// TODO:
// 0   0   0   0   1   x   x   x   x
// 1   0   0   0   0   1   x   x   x
// x   1   0   0   0   0   1   x   x
// x   x   1   0   0   0   0   1   x
// x   x   x   1   0   0   0   0   1

// evenness: 3 weight: 15
// 1   0   0   0   0   1   1   2   4
// 0   1   0   0   0   1   2   1   7
// 0   0   1   0   0   1   3   8   5
// 0   0   0   1   0   1   4   9   8
// 0   0   0   0   1   1   5   3   9

template <typename Badger, typename Block>
inline void Combine5(const Block input[BlockLength(5)], Block output[5]) {
  output[0] = input[0];
  output[1] = input[1];
  output[2] = input[2];
  output[3] = input[3];
  output[4] = input[4];

  output[0] = Plus(output[0], input[5]);
  output[1] = Plus(output[1], input[5]);
  output[2] = Plus(output[2], input[5]);
  output[3] = Plus(output[3], input[5]);
  output[4] = Plus(output[4], input[5]);

  Badger::template Dot5<1, 2, 3, 4, 5>(output, input[6]);
  Badger::template Dot5<2, 1, 8, 9, 3>(output, input[7]);
  Badger::template Dot5<4, 7, 5, 8, 9>(output, input[8]);
}

// evenness: 2 weight: 13
// 1   0   0   0   0   0   1   1   4 
// 0   1   0   0   0   0   1   2   3 
// 0   0   1   0   0   0   1   3   6 
// 0   0   0   1   0   0   1   4   5 
// 0   0   0   0   1   0   1   5   1 
// 0   0   0   0   0   1   1   6   2 
template <typename Badger, typename Block>
inline void Combine6(const Block input[BlockLength(6)], Block output[6]) {
  for (int i = 0; i < 6; ++i) output[i] = Plus(input[i], input[6]);
  Badger::template Dot6<1, 2, 3, 4, 5, 6>(output, input[7]);
  Badger::template Dot6<4, 3, 6, 5, 1, 2>(output, input[8]);
}

template <typename BlockWrapper, int hh_output_length>
static void Hash(const uint64_t *entropy, const unsigned char *char_input, size_t length,
                 uint64_t output[hh_output_length]) {
  constexpr int message_length = MessageLength(hh_output_length);
  constexpr int block_length = BlockLength(hh_output_length);
  static_assert(block_length >= message_length, "Codes don't drop characters");
  constexpr int log2_alphabet = Log2Alphabet(hh_output_length);
  constexpr unsigned kFanout = 8;
  using Block = typename BlockWrapper::Block;
  using Badger =
      EhcBadger<BlockWrapper, log2_alphabet, message_length, block_length, kFanout>;

  Block stack[Badger::kMaxStackSize][kFanout][hh_output_length];
  int stack_lengths[Badger::kMaxStackSize] = {};
  size_t wide_length = length / sizeof(Block) / (message_length * log2_alphabet);

  Badger::DfsTreeHash(char_input, wide_length, stack, stack_lengths, entropy);
  entropy += block_length * log2_alphabet /* <- The amount of entropy used
                                             by EhcBaseLayer() */
             + hh_output_length * (kFanout - 1) * Badger::kMaxStackSize;

  auto used_chars = wide_length * sizeof(Block) * (message_length * log2_alphabet);
  char_input += used_chars;
  Badger::DfsGreedyFinalizer(stack, stack_lengths, char_input, length - used_chars,
                             entropy, output);
}

template <typename BlockWrapper, unsigned count>
struct alignas(alignof(typename BlockWrapper::Block)) Repeat {
  typename BlockWrapper::Block it[count];
};

template <typename InnerBlockWrapper, unsigned count>
struct RepeatWrapper {
  using InnerBlock = typename InnerBlockWrapper::Block;

  using Block = Repeat<InnerBlockWrapper, count>;

  static Block LoadOne(uint64_t entropy) {
    Block result;

    for (unsigned i = 0; i < count; ++i) {
      result.it[i] = InnerBlockWrapper::LoadOne(entropy);
    }
    return result;
  }

  static Block LoadBlock(const void *x) {
    auto y = reinterpret_cast<const unsigned char *>(x);
    Block result;
    for (unsigned i = 0; i < count; ++i) {
      result.it[i] = InnerBlockWrapper::LoadBlock(y + i * sizeof(InnerBlock));
    }
    return result;
  }

  static Block LoadBlockNative(const uint64_t *x) {
    Block result;
    for (unsigned i = 0; i < count; ++i) {
      result.it[i] = InnerBlockWrapper::LoadBlockNative(x + i * sizeof(InnerBlock) /
                                                                sizeof(uint64_t));
    }
    return result;
  }
};

template <typename Block, unsigned count>
inline Repeat<Block, count> Xor(const Repeat<Block, count> &a,
                                const Repeat<Block, count> &b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Xor(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> Plus32(const Repeat<Block, count> &a,
                                   const Repeat<Block, count> &b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Plus32(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> Plus(const Repeat<Block, count> &a,
                                 const Repeat<Block, count> &b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Plus(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> Minus(const Repeat<Block, count> &a,
                                  const Repeat<Block, count> &b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Minus(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> LeftShift(const Repeat<Block, count> &a, int s) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = LeftShift(a.it[i], s);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> RightShift32(const Repeat<Block, count> &a) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = RightShift32(a.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> Times(const Repeat<Block, count> &a,
                                  const Repeat<Block, count> &b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Times(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline uint64_t Sum(const Repeat<Block, count> &a) {
  uint64_t result = 0;
  for (unsigned i = 0; i < count; ++i) {
    result += Sum(a.it[i]);
  }
  return result;
}

}  // namespace

//------------------------------------------------------------
template <typename Wrapper, unsigned hh_output_length>
inline constexpr size_t GetEntropyBytesNeeded(size_t n) {
  return EhcBadger<Wrapper, Log2Alphabet(hh_output_length),
                   MessageLength(hh_output_length),
                   BlockLength(hh_output_length)>::GetEntropyBytesNeeded(n);
}

}  // namespace advanced

static constexpr int kMaxHhOutputLength = 6;
static constexpr int kMaxLogBlockWidth = 4;

template <int log_block_width>
struct FixedWidthBlock {
  static constexpr int kLbw =
      (log_block_width < HALFTIME_IMPL_WIDEST_SIMD_LOG_BLOCK_WIDTH)
          ? log_block_width
          : HALFTIME_IMPL_WIDEST_SIMD_LOG_BLOCK_WIDTH;
  using SimdWrapper = typename advanced::BestSimd<kLbw>::SimdDispatch;
  // using BlockHelper = SimdWrapper::Block;
  // static_assert(sizeof(BlockHelper) > 0, "Is this a real type?");
  using BlockWrapper =
      advanced::RepeatWrapper<SimdWrapper, 1 << (log_block_width - kLbw)>;
};

}  // namespace halftime_hash
