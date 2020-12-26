#include <immintrin.h>

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <type_traits>

namespace halftime_hash {

namespace {

#if __AVX512F__

using u512 = __m512i;

inline u512 Plus(u512 a, u512 b) { return _mm512_add_epi64(a, b); }
inline u512 Plus32(u512 a, u512 b) { return _mm512_add_epi32(a, b); }
inline u512 Times(u512 a, u512 b) { return _mm512_mul_epu32(a, b); }
inline u512 Xor(u512 a, u512 b) { return _mm512_xor_epi32(a, b); }
inline uint64_t Sum(u512 a) { return _mm512_reduce_add_epi64(a); }
inline u512 RightShift32(u512 a) { return _mm512_srli_epi64(a, 32); }
//  inline u512 RightShift32(u512 a, int i) { return _mm512_shuffle_epi32(a,
//  _MM_PERM_ACAC); }
inline u512 LeftShift(u512 a, int i) { return _mm512_slli_epi64(a, i); }
inline u512 Minus(u512 a, u512 b) { return _mm512_sub_epi64(a, b); }
inline u512 Negate(u512 a) { return Minus(_mm512_set1_epi64(0), a); }

struct BlockWrapper512 {
  using Block = u512;
  static Block LoadBlock(const void* x) { return _mm512_loadu_si512(x); }
  static Block LoadOne(uint64_t entropy) { return _mm512_set1_epi64(entropy); }
};

#endif

#if __AVX2__

using u256 = __m256i;

inline u256 Plus(u256 a, u256 b) { return _mm256_add_epi64(a, b); }
inline u256 Plus32(u256 a, u256 b) { return _mm256_add_epi32(a, b); }
inline u256 Times(u256 a, u256 b) { return _mm256_mul_epu32(a, b); }
inline u256 Xor(u256 a, u256 b) { return _mm256_xor_si256(a, b); }
inline u256 LeftShift(u256 a, int i) { return _mm256_slli_epi64(a, i); }
inline u256 RightShift32(u256 a) { return _mm256_srli_epi64(a, 32); }
inline u256 Minus(u256 a, u256 b) { return _mm256_sub_epi64(a, b); }

static inline u256 Negate(u256 a) {
  const auto zero = _mm256_set1_epi64x(0);
  return Minus(zero, a);
}

inline uint64_t Sum(u256 a) {
  auto c = _mm256_extracti128_si256(a, 0);
  auto d = _mm256_extracti128_si256(a, 1);
  c = _mm_add_epi64(c, d);
  return _mm_extract_epi64(c, 0) + _mm_extract_epi64(c, 1);
}

struct BlockWrapper256 {
  using Block = u256;

  static u256 LoadBlock(const void* x) {
    auto y = reinterpret_cast<const u256*>(x);
    return _mm256_loadu_si256(y);
  }

  static u256 LoadOne(uint64_t entropy) { return _mm256_set1_epi64x(entropy); }
};

#endif

#if __SSE2__

struct u128 {

  __m128i payload;

  u128(__m128i payload) : payload(payload) {}

  u128(){};
  explicit u128(uint64_t x) : payload(_mm_set1_epi64x(x)) {}

  u128 operator^(u128 b) const { return {_mm_xor_si128(payload, b.payload)}; }
  void operator^=(u128 that) { *this = *this ^ that; }
  u128 operator+(u128 b) const { return {_mm_add_epi64(payload, b.payload)}; }
  void operator+=(u128 that) { *this = *this + that; }
  u128 operator-(u128 b) const { return {_mm_sub_epi64(payload, b.payload)}; }
  u128 operator-() const { return u128(0ul) - payload; }
  u128 operator>>(int x) const { return {_mm_srli_epi64(payload, x)}; }
  u128 operator<<(int x) const { return {_mm_slli_epi64(payload, x)}; }
};

template <typename T>
T LoadPointer(const void*);

template <>
u128 LoadPointer<u128>(const void* x) {
  return {_mm_loadu_si128(reinterpret_cast<const __m128i*>(x))};
}

u128 Times32(u128 a, u128 b) { return {_mm_mul_epu32(a.payload, b.payload)}; }

u128 Plus32(u128 a, u128 b) { return {_mm_add_epi32(a.payload, b.payload)}; }

uint64_t Sum(u128 a) {
  return _mm_extract_epi64(a.payload, 0) + _mm_extract_epi64(a.payload, 1);
}

#endif

// inline uint64_t Xor(uint64_t a, uint64_t b) { return a ^ b; }
// inline uint64_t Plus(uint64_t a, uint64_t b) { return a + b; }
// inline uint64_t Minus(uint64_t a, uint64_t b) { return a - b; }
// inline uint64_t LeftShift(uint64_t a, int s) { return a << s; }
// inline uint64_t RightShift32(uint64_t a) { return a >> 32; }

inline uint64_t Sum(uint64_t a) { return a; }

// inline uint64_t Negate(uint64_t a) { return -a; }

inline uint64_t Plus32(uint64_t a, uint64_t b) {
  uint64_t result;
  uint32_t temp[2] = {(uint32_t)a + (uint32_t)b,
                      (uint32_t)(a >> 32) + (uint32_t)(b >> 32)};
  memcpy(&result, temp, sizeof(result));
  return result;
}

inline uint64_t Times32(uint64_t a, uint64_t b) {
  constexpr uint64_t mask = (1ul << 32) - 1;
  return (a & mask) * (b & mask);
}

template <>
uint64_t LoadPointer<uint64_t>(const void* x) {
  auto y = reinterpret_cast<const char*>(x);
  uint64_t result;
  memcpy(&result, y, sizeof(uint64_t));
  return result;
}

template <typename Block>
inline void Encode3(Block raw_io[9 * 3]) {
  auto io = reinterpret_cast<Block(*)[3]>(raw_io);
  constexpr unsigned x = 0, y = 1, z = 2;

  const Block* iter = io[0];
  io[7][x] = io[8][x] = iter[x];
  io[7][y] = io[8][y] = iter[y];
  io[7][z] = io[8][z] = iter[z];
  iter += 1;

  auto DistributeRaw = [io, iter](unsigned slot, unsigned label,
                                  std::initializer_list<unsigned> rest) {
    for (unsigned i : rest) {
      io[slot][i] ^= iter[label];
    }
  };

  auto Distribute3 = [&iter, DistributeRaw](unsigned idx,
                                            std::initializer_list<unsigned> a,
                                            std::initializer_list<unsigned> b,
                                            std::initializer_list<unsigned> c) {
    DistributeRaw(idx, x, a);
    DistributeRaw(idx, y, b);
    DistributeRaw(idx, z, c);
    iter += 1;
  };

  while (iter != io[9]) {
    Distribute3(7, {x}, {y}, {z});
  }

  iter = io[1];
  Distribute3(8, {z}, {x, z}, {y});
  Distribute3(8, {x, z}, {x, y, z}, {y, z});
  Distribute3(8, {y}, {y, z}, {x, z});
  Distribute3(8, {x, y}, {z}, {x});
  Distribute3(8, {y, z}, {x, y}, {x, y, z});
  Distribute3(8, {x, y, z}, {x}, {x, y});
}

template <typename Block>
inline void Encode2(Block raw_io[12 * 2]) {
  auto io = reinterpret_cast<Block(*)[2]>(raw_io);
  for (int i = 0; i < 2; ++i) {
    io[11][i] = io[0][i];
    for (int j = 1; j < 11; ++j) {
      io[11][i] ^= io[j][i];
    }
  }
}

// https://docs.switzernet.com/people/emin-gabrielyan/051102-erasure-10-7-resilient/
template <typename Block>
inline void Encode4(Block raw_io[10 * 3]) {
  auto io = reinterpret_cast<Block(*)[3]>(raw_io);

  constexpr unsigned x = 0, y = 1, z = 2;

  const Block* iter = io[0];
  io[7][x] = io[8][x] = io[9][x] = iter[x];
  io[7][y] = io[8][y] = io[9][y] = iter[y];
  io[7][z] = io[8][z] = io[9][z] = iter[z];
  iter += 1;

  auto DistributeRaw = [io, iter](unsigned slot, unsigned label,
                                  std::initializer_list<unsigned> rest) {
    for (unsigned i : rest) {
      io[slot][i] ^= iter[label];
    }
  };

  auto Distribute3 = [&iter, DistributeRaw](unsigned idx,
                                            std::initializer_list<unsigned> a,
                                            std::initializer_list<unsigned> b,
                                            std::initializer_list<unsigned> c) {
    DistributeRaw(idx, x, a);
    DistributeRaw(idx, y, b);
    DistributeRaw(idx, z, c);
    iter += 1;
  };

  while (iter != io[10]) {
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

// https://docs.switzernet.com/people/emin-gabrielyan/051103-erasure-9-5-resilient/
template <typename Block>
inline void Encode5(Block raw_io[9 * 3]) {
  auto io = reinterpret_cast<Block(*)[3]>(raw_io);

  constexpr unsigned x = 0, y = 1, z = 2;

  const Block* iter = io[0];
  io[5][x] = io[6][x] = iter[x];
  io[5][y] = io[6][y] = iter[y];
  io[5][z] = io[6][z] = iter[z];

  io[7][x] = io[8][x] = iter[y];
  io[7][y] = io[8][y] = iter[z];
  io[7][z] = io[8][z] = iter[x] ^ iter[y];
  iter += 1;

  auto DistributeRaw = [io, iter](unsigned slot, unsigned label,
                                  std::initializer_list<unsigned> rest) {
    for (unsigned i : rest) {
      io[slot][i] ^= iter[label];
    }
  };

  auto Distribute3 = [&iter, DistributeRaw](unsigned idx,
                                            std::initializer_list<unsigned> a,
                                            std::initializer_list<unsigned> b,
                                            std::initializer_list<unsigned> c) {
    DistributeRaw(idx, x, a);
    DistributeRaw(idx, y, b);
    DistributeRaw(idx, z, c);
    iter += 1;
  };

  while (iter != io[9]) {
    Distribute3(5, {x}, {y}, {z});
  }

  iter = io[1];
  Distribute3(6, {z}, {x, z}, {y});           // 73
  Distribute3(6, {x, z}, {x, y, z}, {y, z});  // 140
  Distribute3(6, {y}, {y, z}, {x, z});        // 167
  Distribute3(6, {x, y}, {z}, {x});           // 198

  iter = io[1];
  Distribute3(7, {x, y, z}, {x}, {x, y});     // 323
  Distribute3(7, {x, z}, {x, y, z}, {y, z});  // 140
  Distribute3(7, {x}, {y}, {z});              // 11
  Distribute3(7, {y}, {y, z}, {x, z});        // 167

  iter = io[1];
  Distribute3(8, {x}, {y}, {z});              // 11
  Distribute3(8, {x, y}, {z}, {x});           // 198
  Distribute3(8, {y, z}, {x, y}, {x, y, z});  // 292
  Distribute3(8, {x, z}, {x, y, z}, {y, z});  // 140
}

template <typename Badger, typename Block>
inline void Combine2(const Block input[12], Block output[2]);

template <typename Badger, typename Block>
inline void Combine3(const Block input[9], Block output[3]);

template <typename Badger, typename Block>
inline void Combine4(const Block input[10], Block output[3]);

template <typename Badger, typename Block>
inline void Combine5(const Block input[9], Block output[3]);

constexpr inline int CeilingLog2(size_t n) {
  return (n <= 1) ? 0 : 1 + CeilingLog2(n / 2 + n % 2);
}

template <typename Block, unsigned dimension, unsigned in_width,
          unsigned encoded_dimension, unsigned out_width, unsigned fanout = 8>
struct EhcBadger {
  static_assert(alignof(Block) == sizeof(Block), "alignof(Block) == sizeof(Block)");

  static Block Mix(Block input, Block entropy) {
    Block output = Plus32(entropy, input);
    Block twin = output >> 32;
    output = Times32(output, twin);
    return output;
  }

  static Block MixOne(Block input, uint64_t entropy) {
    return Mix(input, Block(entropy));
  }

  static void EhcUpperLayer(const Block (&input)[fanout][out_width],
                            const uint64_t entropy[out_width * (fanout - 1)],
                            Block (&output)[out_width]) {
    for (unsigned i = 0; i < out_width; ++i) {
      output[i] = input[0][i];
      for (unsigned j = 1; j < fanout; ++j) {
        output[i] += MixOne(input[j][i], entropy[(fanout - 1) * i + j - 1]);
      }
    }
  }

  static void Encode(Block io[encoded_dimension][in_width]) {
    static_assert(2 <= out_width && out_width <= 5, "uhoh");
    if (out_width == 3) return Encode3<Block>(&io[0][0]);
    if (out_width == 2) return Encode2<Block>(&io[0][0]);
    if (out_width == 4) return Encode4<Block>(&io[0][0]);
    if (out_width == 5) return Encode5<Block>(&io[0][0]);
  }

  static Block SimpleTimes(std::integral_constant<int, -1>, Block x) { return -x; }
  static Block SimpleTimes(std::integral_constant<int, 1>, Block x) { return x; }
  static Block SimpleTimes(std::integral_constant<int, 2>, Block x) { return x << 1; }
  static Block SimpleTimes(std::integral_constant<int, 3>, Block x) {
    return x + (x << 1);
  }
  static Block SimpleTimes(std::integral_constant<int, 4>, Block x) { return x << 2; }
  static Block SimpleTimes(std::integral_constant<int, 5>, Block x) {
    return x + (x << 2);
  }
  static Block SimpleTimes(std::integral_constant<int, 7>, Block x) {
    return (x << 3) - x;
  }
  static Block SimpleTimes(std::integral_constant<int, 8>, Block x) { return x << 3; }
  static Block SimpleTimes(std::integral_constant<int, 9>, Block x) {
    return x + (x << 3);
  }

  template <int a>
  static Block SimplerTimes(Block x) {
    return SimpleTimes(std::integral_constant<int, a>{}, x);
  }

  template <int a, int b>
  static void Dot2(Block sinks[2], Block x) {
    sinks[0] +=  SimplerTimes<a>(x);
    sinks[1] += SimplerTimes<b>(x);
  }

  template <int a, int b, int c>
  static void Dot3(Block sinks[3], Block x) {
    Dot2<a, b>(sinks, x);
    sinks[2] += SimplerTimes<c>(x);
  }

  template <int a, int b, int c, int d>
  static void Dot4(Block sinks[4], Block x) {
    Dot3<a, b, c>(sinks, x);
    sinks[3] += SimplerTimes<d>(x);
  }

  template <int a, int b, int c, int d, int e>
  static void Dot5(Block sinks[5], Block x) {
    Dot4<a, b, c, d>(sinks, x);
    sinks[4] += SimplerTimes<e>(x);
  }

  static void Combine(const Block input[encoded_dimension], Block (&output)[out_width]) {
    if (out_width == 3) return Combine3<EhcBadger>(input, output);
    if (out_width == 2) return Combine2<EhcBadger>(input, output);
    if (out_width == 4) return Combine4<EhcBadger>(input, output);
    if (out_width == 5) return Combine5<EhcBadger>(input, output);
  }

  static void Load(const char input[dimension * in_width * sizeof(Block)],
                   Block output[dimension][in_width]) {
    static_assert(dimension * in_width <= 28, "");
#ifndef __clang__
#pragma GCC unroll 28
#else
#pragma unroll
#endif
    for (unsigned i = 0; i < dimension; ++i) {
#ifndef __clang__
#pragma GCC unroll 28
#else
#pragma unroll
#endif
      for (unsigned j = 0; j < in_width; ++j) {
        output[i][j] = LoadPointer<Block>(&input[(i * in_width + j) * sizeof(Block)]);
      }
    }
  }

  // TODO: input should be 2d to prevent mistakes like iteration order switching
  static void Hash(const Block (&input)[encoded_dimension][in_width],
                   const uint64_t entropy[encoded_dimension][in_width],
                   Block output[encoded_dimension]) {
    // auto a = input[0];
    // auto b = entropy[0];
    for (unsigned i = 0; i < encoded_dimension; ++i) {
      output[i] = MixOne(input[i][0], entropy[i][0]);
      // TODO: should loading take care of this?
    }
    for (unsigned j = 1; j < in_width; ++j) {
      // a = &input[j];
      // b = &entropy[j];
      for (unsigned i = 0; i < encoded_dimension; ++i) {
        // TODO: reduce entropy index here
        output[i] += MixOne(input[i][j], entropy[i][j]);
        // a += in_width;
        // TODO: this might be optional; it might not matter which way we iterate over
        // entropy
        // b += in_width;
      }
    }
  }

  static void EhcBaseLayer(const char input[dimension * in_width * sizeof(Block)],
                           const uint64_t raw_entropy[encoded_dimension][in_width],
                           Block (&output)[out_width]) {
    Block scratch[encoded_dimension][in_width];
    Block tmpout[encoded_dimension];
    Load(input, scratch);
    Encode(scratch);
    Hash(scratch, raw_entropy, tmpout);
    Combine(tmpout, output);
  }

  static void DfsTreeHash(const char* data, size_t block_group_length,
                          Block stack[][fanout][out_width], int stack_lengths[],
                          const uint64_t* entropy) {
    auto entropy_matrix = reinterpret_cast<const uint64_t(*)[in_width]>(entropy);
    for (size_t k = 0; k < block_group_length; ++k) {
      int i = 0;
      while (stack_lengths[i] == fanout) ++i;
      for (int j = i - 1; j >= 0; --j) {
        EhcUpperLayer(
            stack[j],
            &entropy[encoded_dimension * in_width + (fanout - 1) * out_width * j],
            stack[j + 1][stack_lengths[j + 1]]);
        stack_lengths[j] = 0;
        stack_lengths[j + 1] += 1;
      }

      EhcBaseLayer(&data[k * dimension * in_width * sizeof(Block)], entropy_matrix,
                   stack[0][stack_lengths[0]]);
      stack_lengths[0] += 1;
    }
  }

  static constexpr uint64_t FloorLog(uint64_t a, uint64_t b) {
    return (0 == a) ? 0 : ((b < a) ? 0 : 1 + (FloorLog(a, b / a)));
  }

  static constexpr size_t GetEntropyBytesNeeded(size_t n) {
    auto b = sizeof(Block) / sizeof(uint64_t);
    auto h = FloorLog(fanout, n / b / dimension / in_width);
    return sizeof(uint64_t) *
           (encoded_dimension * (in_width - 1) + (fanout - 1) * out_width * h +
            b * fanout * out_width * h + b * dimension * in_width + out_width - 1);
  }

  struct BlockGreedy {
   private:
    const uint64_t* seeds;
    Block accum[out_width] = {};

   public:
    BlockGreedy(const uint64_t seeds[]) : seeds(seeds) {}

    void Insert(const Block (&x)[out_width]) {
      for (unsigned i = 0; i < out_width; ++i) {
        accum[i] += Mix(x[i], LoadPointer<Block>(seeds));
        seeds += sizeof(Block) / sizeof(uint64_t);
      }
    }

    void Insert(Block x) {
      for (unsigned i = 0; i < out_width; ++i) {
        accum[i] += accum[i],
            Mix(x, LoadPointer<Block>(&seeds[i * sizeof(Block) / sizeof(uint64_t)]));
      }
      // Toeplitz
      seeds += sizeof(Block) / sizeof(uint64_t);
    }

    void Hash(uint64_t output[out_width]) const {
      for (unsigned i = 0; i < out_width; ++i) {
        output[i] = Sum(accum[i]);
      }
    }
  };

  static void DfsGreedyFinalizer(const Block stack[][fanout][out_width],
                                 const int stack_lengths[], const char* char_input,
                                 size_t char_length, const uint64_t* entropy,
                                 uint64_t output[out_width]) {
    BlockGreedy b(entropy);
    for (int j = 0; stack_lengths[j] > 0; ++j) {
      for (int k = 0; k < stack_lengths[j]; k += 1) {
        b.Insert(stack[j][k]);
      }
    }

    size_t i = 0;
    for (; i + sizeof(Block) <= char_length; i += sizeof(Block)) {
      b.Insert(LoadPointer<Block>(&char_input[i]));
    }

    if (1) {
      Block extra = Block();
      memcpy(&extra, &char_input[i], char_length - i);
      b.Insert(extra);
    } else {
      Block extra;
      char* extra_char = reinterpret_cast<char*>(&extra);
      for (unsigned j = 0; j < sizeof(Block); ++j) {
        if (j < char_length - i) {
          extra_char[j] = char_input[i + j];
        } else {
          extra_char[j] = 0;
        }
      }
      b.Insert(extra);
    }
    b.Hash(output);
  }
};  // EhcBadger

// %  1   0   0   1   1   1  17   1   4
// %  0   1   0  -2  18   4  -1   1   1
// %  0   0   1   3   2   9   5  20   2
//
// -2, -1, 0, 1, 2, 3, 4, 5, 9, 17, 18 20

// Consider:
// evenness: 3 weight: 11
//  0   0   1   2   1   1   1   2   4
//  1   1   0   0   1   2   1  10   1
//  1   2   1   1   0   0   1   9   8

// evenness: 2 weight: 12
//  0   0   1   4   1   1   1  10   2
//  1   1   0   0   1   4   2   1   2
//  1   4   1   1   0   0   2  10   1

// evenness: 2 weight: 10
//  0   0   1   4   1   1   2   2   1
//  1   1   0   0   1   4   1   2   2
//  1   4   1   1   0   0   2   1   2

template <typename Badger, typename Block>
inline void Combine3(const Block input[9], Block output[3]) {
  output[1] = input[0];
  output[2] = input[0];

  output[1] += input[1];
  output[2] += input[1] << 2;

  output[0] = input[2];
  output[2] += input[2];

  output[0] += input[3] << 2;
  output[2] += input[3];

  output[0] += input[4];
  output[1] += input[4];

  output[0] += input[5];
  output[1] += input[5] << 2;

  Badger::template Dot3<2, 1, 2>(output, input[6]);
  Badger::template Dot3<2, 2, 1>(output, input[7]);
  Badger::template Dot3<1, 2, 2>(output, input[8]);
}

// template <typename Badger, typename Block>
// inline void Combine3(const Block input[9], Block output[3]) {
//   output[0] = input[0];
//   output[1] = input[1];
//   output[2] = input[2];

//   Badger::template Dot3<1, -2, 3>(output, input[3]);
//   Badger::template Dot3<1, 18, 2>(output, input[4]);
//   Badger::template Dot3<1, 4, 9>(output, input[5]);
//   Badger::template Dot3<17, -1, 5>(output, input[6]);
//   Badger::template Dot3<1, 1, 20>(output, input[7]);
//   Badger::template Dot3<4, 1, 2>(output, input[8]);
// }

template <typename Badger, typename Block>
inline void Combine2(const Block input[12], Block output[2]) {
  output[0] = input[0];
  output[1] = input[1];

  Badger::template Dot2<1, 1>(output, input[3]);
  Badger::template Dot2<1, -1>(output, input[4]);
  Badger::template Dot2<1, 2>(output, input[5]);
  Badger::template Dot2<2, 1>(output, input[6]);
  Badger::template Dot2<-1, 2>(output, input[7]);
  Badger::template Dot2<2, -1>(output, input[8]);
  Badger::template Dot2<1, 3>(output, input[9]);
  Badger::template Dot2<1, 4>(output, input[10]);
  Badger::template Dot2<4, 1>(output, input[11]);
  Badger::template Dot2<1, 5>(output, input[1]);
}

//
// evenness: 4 weight: 16
//   8   8   0   2   1   8   2   1   2   4
//   0   8   1   0   1   1   4   1   4   2
//   1   8   1   4   2   8   1   4   1   2
//   8   1   1   1   1   8   1   8   4   1
//
// evenness: 3 weight: 25
//   8   7   1   9   0   0   1   9   0   1
//   9   8   0   8   9   9   3   5   0   0
//   9   4   0   5   0   1   8   3   1   0
//   8   9   9   0   0   9   4   4   4   0
//
// shifts: 14
// adds: 11
// evenness: 3 weight: 24
//   0   9   1   4   8   0   0   7   3   9
//   9   4   3   0   8   1   0   9   0   9
//   9   1   0   4   1   0   5   7   1   8
//   8   2   7   1   1   3   0   3   1   0
//
// shifts: 15
// adds: 9
// evenness: 3 weight: 23
//  0   7   7   8   1   0   9   9   4   0
//  1   8   4   4   2   0   0   9   1   0
//  0   0   1   1   7   4   1   0   1   1
//  9   0   1   0   3   7   8   0   7   3
//
// shifts: 13
// adds: 11
// evenness: 3 weight: 22
//   3   0   7   9   1   0   8   0   7   9
//   3   1   9   0   2   0   1   2   1   0
//   0   0   7   5   2   0   1   3   9   4
//   0   1   0   4   7   7   1   7   7   0
//
// rearranged:
//  0 0 0 3 9 7 9 1 8 7
//  0 1 2 3 0 9 0 2 1 1
//  0 0 3 0 4 7 5 2 1 9
//  7 1 7 0 0 0 4 7 1 7
//
// shifts: 16
// adds: 7
// negations: 2
// evenness: 3 weight: 24
//  1   0   0   0   1   1  17   2   4 -17
//  0   1   0   0   1   2   4   1   3   4
//  0   0   1   0   1   3   9   8   1   8
//  0   0   0   1   1   4 -10   4  10   3
//

// NEW!
// evenness: 3 weight: 21
// 0   0   0   1   1   4   2   4   1   1
// 0   1   2   0   0   1   1   2   4   1
// 2   0   1   0   4   0   1   1   1   1
// 1   1   0   1   0   0   4   1   2   8

template <typename Badger, typename Block>
inline void Combine4(const Block input[10], Block output[4]) {
  output[2] = input[0] << 1;
  output[3] = input[0];

  output[1] = input[1];
  output[3] += input[1];

  output[1] += input[2] << 1;
  output[2] += input[2];

  output[0] = input[3];
  output[3] += input[3];

  output[0] += input[4];
  output[2] += input[4] << 2;

  output[0] += input[5] << 2;
  output[1] += input[5];

  Badger::template Dot4<2, 1, 1, 4>(output, input[6]);
  Badger::template Dot4<4, 2, 1, 1>(output, input[7]);
  Badger::template Dot4<1, 4, 1, 2>(output, input[8]);
  Badger::template Dot4<1, 1, 1, 8>(output, input[9]);
}

// template <typename Badger, typename Block>
// inline void Combine4(const Block input[10], Block output[4]) {
//   output[0] = input[0];
//   output[1] = input[1];
//   output[2] = input[2];
//   output[3] = input[3];

//   output[0] = Plus(output[0], input[4]);
//   output[1] = Plus(output[1], input[4]);
//   output[2] = Plus(output[2], input[4]);
//   output[3] = Plus(output[3], input[4]);

//   Badger::template Dot4<1, 2, 3, 4>(output, input[5]);
//   Badger::template Dot4<17, 4, 9, -10>(output, input[6]);
//   Badger::template Dot4<2, 1, 8, 4>(output, input[7]);
//   Badger::template Dot4<4, 3, 1, 10>(output, input[8]);
//   Badger::template Dot4<-17, 4, 8, 3>(output, input[9]);
// }

// 0   0   0   0   1   1   5   3   9
// 1   0   0   0   0   1   1   2   4
// x   1   0   0   0   0   2   1   7
// x   x   1   0   0   0   0   8   5
// x   x   x   1   0   0   0   0   8


  
// evenness: 3 weight: 15
// 1   0   0   0   0   1   1   2   4
// 0   1   0   0   0   1   2   1   7
// 0   0   1   0   0   1   3   8   5
// 0   0   0   1   0   1   4   9   8
// 0   0   0   0   1   1   5   3   9


template <typename Badger, typename Block>
inline void Combine5(const Block input[10], Block output[5]) {
  output[0] = input[0];
  output[1] = input[1];
  output[2] = input[2];
  output[3] = input[3];
  output[4] = input[4];

  output[0] += input[5];
  output[1] += input[5];
  output[2] += input[5];
  output[3] += input[5];
  output[4] += input[5];

  Badger::template Dot5<1, 2, 3, 4, 5>(output, input[6]);
  Badger::template Dot5<2, 1, 8, 9, 3>(output, input[7]);
  Badger::template Dot5<4, 7, 5, 8, 9>(output, input[8]);
}

template <int width>
inline uint64_t TabulateBytes(uint64_t input, const uint64_t entropy[256 * width]) {
  const uint64_t(&table)[width][256] =
      *reinterpret_cast<const uint64_t(*)[width][256]>(entropy);
  uint64_t result = 0;
  for (unsigned i = 0; i < width; ++i) {
    uint8_t index = input >> (i * CHAR_BIT);
    result ^= table[i][index];
  }
  return result;
}

template <typename Block, unsigned dimension, unsigned in_width,
          unsigned encoded_dimension, unsigned out_width>
void Hash(const uint64_t* entropy, const char* char_input, size_t length,
          uint64_t output[out_width]) {
  constexpr unsigned kMaxStackSize = 9;
  constexpr unsigned kFanout = 8;

  Block stack[kMaxStackSize][kFanout][out_width];
  int stack_lengths[kMaxStackSize] = {};
  size_t wide_length = length / sizeof(Block) / (dimension * in_width);

  EhcBadger<Block, dimension, in_width, encoded_dimension, out_width,
            kFanout>::DfsTreeHash(char_input, wide_length, stack, stack_lengths, entropy);
  entropy += encoded_dimension * in_width + out_width * (kFanout - 1) * kMaxStackSize;

  auto used_chars = wide_length * sizeof(Block) * (dimension * in_width);
  char_input += used_chars;

  EhcBadger<Block, dimension, in_width, encoded_dimension, out_width,
            kFanout>::DfsGreedyFinalizer(stack, stack_lengths, char_input,
                                         length - used_chars, entropy, output);
}

// template <typename Block, unsigned count>
// struct alignas(sizeof(Block) * count) Repeat {
//   static_assert(sizeof(Block) == alignof(Block), "sizeof(Block) == alignof(Block)");
//   Block it[count];

//   Repeat operator*(long x) const {
//     Repeat result;
//     for (unsigned i = 0; i < count; ++i) {
//       result.it[i] = it[i] * x;
//     }
//     return result;
//   }
// };

// template <typename Block, unsigned count>
// inline Repeat<Block, count> operator*(long x, Repeat<Block, count> here) {
//   Repeat<Block, count> result;
//   for (unsigned i = 0; i < count; ++i) {
//     result.it[i] = here.it[i] * x;
//   }
//   return result;
// }

// template <typename InnerBlock, unsigned count>
// struct RepeatWrapper {
//   static_assert(sizeof(InnerBlock) == alignof(InnerBlock),
//                 "sizeof(InnerBlock) == alignof(InnerBlock)");

//   using Block = Repeat<InnerBlock, count>;

//   static Block LoadOne(uint64_t entropy) {
//     Block result;
//     for (unsigned i = 0; i < count; ++i) {
//       result.it[i] = InnerBlock(entropy);
//     }
//     return result;
//   }

//   static Block LoadBlock(const void* x) {
//     auto y = reinterpret_cast<const char*>(x);
//     Block result;
//     for (unsigned i = 0; i < count; ++i) {
//       result.it[i] = InnerBlock(y + i * sizeof(InnerBlock));
//     }
//     return result;
//   }
// };

// template <typename Block, unsigned count>
// inline Repeat<Block, count> Xor(Repeat<Block, count> a, Repeat<Block, count> b) {
//   Repeat<Block, count> result;
//   for (unsigned i = 0; i < count; ++i) {
//     result.it[i] = Xor(a.it[i], b.it[i]);
//   }
//   return result;
// }

// template <typename Block, unsigned count>
// inline Repeat<Block, count> Plus32(Repeat<Block, count> a, Repeat<Block, count> b) {
//   Repeat<Block, count> result;
//   for (unsigned i = 0; i < count; ++i) {
//     result.it[i] = Plus32(a.it[i], b.it[i]);
//   }
//   return result;
// }

// template <typename Block, unsigned count>
// inline Repeat<Block, count> Plus(Repeat<Block, count> a, Repeat<Block, count> b) {
//   Repeat<Block, count> result;
//   for (unsigned i = 0; i < count; ++i) {
//     result.it[i] = Plus(a.it[i], b.it[i]);
//   }
//   return result;
// }

// template <typename Block, unsigned count>
// inline Repeat<Block, count> Minus(Repeat<Block, count> a, Repeat<Block, count> b) {
//   Repeat<Block, count> result;
//   for (unsigned i = 0; i < count; ++i) {
//     result.it[i] = Minus(a.it[i], b.it[i]);
//   }
//   return result;
// }

// template <typename Block, unsigned count>
// inline Repeat<Block, count> LeftShift(Repeat<Block, count> a, int s) {
//   Repeat<Block, count> result;
//   for (unsigned i = 0; i < count; ++i) {
//     result.it[i] = LeftShift(a.it[i], s);
//   }
//   return result;
// }

// template <typename Block, unsigned count>
// inline Repeat<Block, count> RightShift32(Repeat<Block, count> a) {
//   Repeat<Block, count> result;
//   for (unsigned i = 0; i < count; ++i) {
//     result.it[i] = RightShift32(a.it[i]);
//   }
//   return result;
// }

// template <typename Block, unsigned count>
// inline Repeat<Block, count> Times(Repeat<Block, count> a, Repeat<Block, count> b) {
//   Repeat<Block, count> result;
//   for (unsigned i = 0; i < count; ++i) {
//     result.it[i] = Times(a.it[i], b.it[i]);
//   }
//   return result;
// }

// template <typename Block, unsigned count>
// inline uint64_t Sum(Repeat<Block, count> a) {
//   uint64_t result = 0;
//   for (unsigned i = 0; i < count; ++i) {
//     result += Sum(a.it[i]);
//   }
//   return result;
// }

// template <typename Block, unsigned count>
// inline Repeat<Block, count> Negate(Repeat<Block, count> a) {
//   Repeat<Block, count> b;
//   for (unsigned i = 0; i < count; ++i) {
//     b.it[i] = Negate(a.it[i]);
//   }
//   return b;
// }

}  // namespace

template <void (*Hasher)(const uint64_t* entropy, const char* char_input, size_t length,
                         uint64_t output[]),
          int width>
inline uint64_t TabulateAfter(const uint64_t* entropy, const char* char_input,
                              size_t length) {
  const uint64_t(&table)[1 + width][256] =
      *reinterpret_cast<const uint64_t(*)[1 + width][256]>(entropy);
  entropy += width * 256;
  uint64_t output[width];
  Hasher(entropy, char_input, length, output);
  uint64_t result = TabulateBytes<sizeof(length)>(length, &table[0][0]);
  for (int i = 0; i < width; ++i) {
    result ^= TabulateBytes<sizeof(output[i])>(output[i], &table[8 * (i + 1)][0]);
  }
  return result;
}

#if __AVX512F__

template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
          unsigned out_width>
inline void V4Avx512(const uint64_t* entropy, const char* char_input, size_t length,
                     uint64_t output[out_width]) {
  return Hash<BlockWrapper512, dimension, in_width, encoded_dimension, out_width>(
      entropy, char_input, length, output);
}

#endif

#if __AVX2__

template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
          unsigned out_width>
inline void V3Avx2(const uint64_t* entropy, const char* char_input, size_t length,
                   uint64_t output[out_width]) {
  return Hash<BlockWrapper256, dimension, in_width, encoded_dimension, out_width>(
      entropy, char_input, length, output);
}

// template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
//           unsigned out_width>
// inline void V4Avx2(const uint64_t* entropy, const char* char_input, size_t length,
//                    uint64_t output[out_width]) {
//   return Hash<RepeatWrapper<BlockWrapper256, 2>, dimension, in_width, encoded_dimension,
//               out_width>(entropy, char_input, length, output);
// }

#endif

#if __SSE2__

template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
          unsigned out_width>
inline void V2Sse2(const uint64_t* entropy, const char* char_input, size_t length,
                   uint64_t output[out_width]) {
  return Hash<u128, dimension, in_width, encoded_dimension, out_width>(
      entropy, char_input, length, output);
}

// template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
//           unsigned out_width>
// inline void V3Sse2(const uint64_t* entropy, const char* char_input, size_t length,
//                    uint64_t output[out_width]) {
//   return Hash<RepeatWrapper<u128, 2>, dimension, in_width, encoded_dimension,
//               out_width>(entropy, char_input, length, output);
// }

// template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
//           unsigned out_width>
// inline void V4Sse2(const uint64_t* entropy, const char* char_input, size_t length,
//                    uint64_t output[out_width]) {
//   return Hash<RepeatWrapper<u128, 4>, dimension, in_width, encoded_dimension,
//               out_width>(entropy, char_input, length, output);
// }

#endif

// template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
//           unsigned out_width>
// inline void V4Scalar(const uint64_t* entropy, const char* char_input, size_t length,
//                      uint64_t output[out_width]) {
//   return Hash<RepeatWrapper<BlockWrapperScalar, 8>, dimension, in_width,
//               encoded_dimension, out_width>(entropy, char_input, length, output);
// }

// template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
//           unsigned out_width>
// inline void V3Scalar(const uint64_t* entropy, const char* char_input, size_t length,
//                      uint64_t output[out_width]) {
//   return Hash<RepeatWrapper<BlockWrapperScalar, 4>, dimension, in_width,
//               encoded_dimension, out_width>(entropy, char_input, length, output);
// }

// template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
//           unsigned out_width>
// inline void V2Scalar(const uint64_t* entropy, const char* char_input, size_t length,
//                      uint64_t output[out_width]) {
//   return Hash<RepeatWrapper<BlockWrapperScalar, 2>, dimension, in_width,
//               encoded_dimension, out_width>(entropy, char_input, length, output);
// }

template <unsigned dimension, unsigned in_width, unsigned encoded_dimension,
          unsigned out_width>
inline void V1Scalar(const uint64_t* entropy, const char* char_input, size_t length,
                     uint64_t output[out_width]) {
  return Hash<uint64_t, dimension, in_width, encoded_dimension, out_width>(
      entropy, char_input, length, output);
}

template <unsigned out_width>
inline constexpr size_t GetEntropyBytesNeeded(size_t n) {
  return (3 == out_width)
             ? EhcBadger<uint64_t, 7, 3, 9, out_width>::GetEntropyBytesNeeded(n)
             : EhcBadger<uint64_t, 11, 2, 12, out_width>::GetEntropyBytesNeeded(
                   n);
}

template <unsigned out_width>
inline void V4(const uint64_t* entropy, const char* char_input, size_t length,
               uint64_t output[out_width]);
template <unsigned out_width>
inline void V3(const uint64_t* entropy, const char* char_input, size_t length,
               uint64_t output[out_width]);
template <unsigned out_width>
inline void V2(const uint64_t* entropy, const char* char_input, size_t length,
               uint64_t output[out_width]);
template <unsigned out_width>
inline void V1(const uint64_t* entropy, const char* char_input, size_t length,
               uint64_t output[out_width]);

#if __AVX512F__

template <>
inline void V4<5>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[5]) {
  return V4Avx512<5, 3, 9, 5>(entropy, char_input, length, output);
}

template <>
inline void V4<4>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[4]) {
  return V4Avx512<7, 3, 10, 4>(entropy, char_input, length, output);
}

template <>
inline void V4<3>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[3]) {
  return V4Avx512<7, 3, 9, 3>(entropy, char_input, length, output);
}

template <>
inline void V4<2>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[2]) {
  return V4Avx512<11, 2, 12, 2>(entropy, char_input, length, output);
}

#elif __AVX2__

template <>
inline void V4<5>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[5]) {
  return V4Avx2<5, 3, 9, 5>(entropy, char_input, length, output);
}

template <>
inline void V4<4>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[4]) {
  return V4Avx2<7, 3, 10, 4>(entropy, char_input, length, output);
}

template <>
inline void V4<3>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[3]) {
  return V4Avx2<7, 3, 9, 3>(entropy, char_input, length, output);
}

template <>
inline void V4<2>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[2]) {
  return V4Avx2<11, 2, 12, 2>(entropy, char_input, length, output);
}

#elif __SSE2__

// template <>
// inline void V4<5>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[5]) {
//   return V4Sse2<5, 3, 9, 5>(entropy, char_input, length, output);
// }

// template <>
// inline void V4<4>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[4]) {
//   return V4Sse2<7, 3, 10, 4>(entropy, char_input, length, output);
// }

// template <>
// inline void V4<3>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[3]) {
//   return V4Sse2<7, 3, 9, 3>(entropy, char_input, length, output);
// }

// template <>
// inline void V4<2>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[2]) {
//   return V4Sse2<11, 2, 12, 2>(entropy, char_input, length, output);
// }

#else

// template <>
// inline void V4<5>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[5]) {
//   return V4Scalar<5, 3, 9, 5>(entropy, char_input, length, output);
// }

// template <>
// inline void V4<4>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[4]) {
//   return V4Scalar<7, 3, 10, 4>(entropy, char_input, length, output);
// }

// template <>
// inline void V4<3>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[3]) {
//   return V4Scalar<7, 3, 9, 3>(entropy, char_input, length, output);
// }

// template <>
// inline void V4<2>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[2]) {
//   return V4Scalar<11, 2, 12, 2>(entropy, char_input, length, output);
// }

#endif

#if __AVX2__

template <>
inline void V3<5>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[5]) {
  return V3Avx2<5, 3, 9, 5>(entropy, char_input, length, output);
}

template <>
inline void V3<4>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[4]) {
  return V3Avx2<7, 3, 10, 4>(entropy, char_input, length, output);
}

template <>
inline void V3<3>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[3]) {
  return V3Avx2<7, 3, 9, 3>(entropy, char_input, length, output);
}

template <>
inline void V3<2>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[2]) {
  return V3Avx2<11, 2, 12, 2>(entropy, char_input, length, output);
}

#elif __SSE2__

// template <>
// inline void V3<5>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[5]) {
//   return V3Sse2<5, 3, 9, 5>(entropy, char_input, length, output);
// }

// template <>
// inline void V3<4>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[4]) {
//   return V3Sse2<7, 3, 10, 4>(entropy, char_input, length, output);
// }

// template <>
// inline void V3<3>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[3]) {
//   return V3Sse2<7, 3, 9, 3>(entropy, char_input, length, output);
// }

// template <>
// inline void V3<2>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[2]) {
//   return V3Sse2<11, 2, 12, 2>(entropy, char_input, length, output);
// }

#else

// template <>
// inline void V3<5>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[5]) {
//   return V3Scalar<5, 3, 9, 5>(entropy, char_input, length, output);
// }

// template <>
// inline void V3<4>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[4]) {
//   return V3Scalar<7, 3, 10, 4>(entropy, char_input, length, output);
// }

// template <>
// inline void V3<3>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[3]) {
//   return V3Scalar<7, 3, 9, 3>(entropy, char_input, length, output);
// }

// template <>
// inline void V3<2>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[2]) {
//   return V3Scalar<11, 2, 12, 2>(entropy, char_input, length, output);
// }

#endif

#if __SSE2__

template <>
inline void V2<5>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[5]) {
  return V2Sse2<5, 3, 9, 5>(entropy, char_input, length, output);
}

template <>
inline void V2<4>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[4]) {
  return V2Sse2<7, 3, 10, 4>(entropy, char_input, length, output);
}

template <>
inline void V2<3>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[3]) {
  return V2Sse2<7, 3, 9, 3>(entropy, char_input, length, output);
}

template <>
inline void V2<2>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[2]) {
  return V2Sse2<11, 2, 12, 2>(entropy, char_input, length, output);
}

#else

// template <>
// inline void V2<5>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[5]) {
//   return V2Scalar<5, 3, 9, 5>(entropy, char_input, length, output);
// }

// template <>
// inline void V2<4>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[4]) {
//   return V2Scalar<7, 3, 10, 4>(entropy, char_input, length, output);
// }

// template <>
// inline void V2<3>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[3]) {
//   return V2Scalar<7, 3, 9, 3>(entropy, char_input, length, output);
// }

// template <>
// inline void V2<2>(const uint64_t* entropy, const char* char_input, size_t length,
//                   uint64_t output[2]) {
//   return V2Scalar<11, 2, 12, 2>(entropy, char_input, length, output);
// }

#endif

template <>
inline void V1<5>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[5]) {
  return V1Scalar<5, 3, 9, 5>(entropy, char_input, length, output);
}

template <>
inline void V1<4>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[4]) {
  return V1Scalar<7, 3, 10, 4>(entropy, char_input, length, output);
}

template <>
inline void V1<3>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[3]) {
  return V1Scalar<7, 3, 9, 3>(entropy, char_input, length, output);
}

template <>
inline void V1<2>(const uint64_t* entropy, const char* char_input, size_t length,
                  uint64_t output[2]) {
  return V1Scalar<11, 2, 12, 2>(entropy, char_input, length, output);
}

}  // namespace halftime_hash
