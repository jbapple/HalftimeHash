#include <cstdint>

namespace halftime_hash {

template <typename T>
T LoadPointer(const void*);

template <>
uint64_t LoadPointer<uint64_t>(const void* x) {
  auto y = reinterpret_cast<const char*>(x);
  uint64_t result;
  memcpy(&result, y, sizeof(uint64_t));
  return result;
}

inline uint64_t Sum(uint64_t a) { return a; }

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

template <typename Block, unsigned count>
struct alignas(sizeof(Block) * count) Repeat {
  static_assert(sizeof(Block) == alignof(Block), "sizeof(Block) == alignof(Block)");
  Block it[count];

  Repeat(){};
  explicit Repeat(uint64_t x) {
    for (unsigned i = 0; i < count; ++i) {
      it[i] = Block(x);
    }
  }

  u128 operator^(u128 b) const { return {_mm_xor_si128(it, b.it)}; }
  void operator^=(u128 that) { *this = *this ^ that; }
  u128 operator+(u128 b) const { return {_mm_add_epi64(it, b.it)}; }
  void operator+=(u128 that) { *this = *this + that; }
  u128 operator-(u128 b) const { return {_mm_sub_epi64(it, b.it)}; }
  u128 operator-() const { return u128(0ul) - it; }
  u128 operator>>(int x) const { return {_mm_srli_epi64(it, x)}; }
  u128 operator<<(int x) const { return {_mm_slli_epi64(it, x)}; }

  
  Repeat operator*(long x) const {
    Repeat result;
    for (unsigned i = 0; i < count; ++i) {
      result.it[i] = it[i] * x;
    }
    return result;
  }
};

template <typename Block, unsigned count>
inline Repeat<Block, count> operator*(long x, Repeat<Block, count> here) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = here.it[i] * x;
  }
  return result;
}

template <typename InnerBlock, unsigned count>
struct RepeatWrapper {
  static_assert(sizeof(InnerBlock) == alignof(InnerBlock),
                "sizeof(InnerBlock) == alignof(InnerBlock)");

  using Block = Repeat<InnerBlock, count>;

  static Block LoadOne(uint64_t entropy) {
    Block result;
    for (unsigned i = 0; i < count; ++i) {
      result.it[i] = InnerBlock(entropy);
    }
    return result;
  }

  static Block LoadBlock(const void* x) {
    auto y = reinterpret_cast<const char*>(x);
    Block result;
    for (unsigned i = 0; i < count; ++i) {
      result.it[i] = InnerBlock(y + i * sizeof(InnerBlock));
    }
    return result;
  }
};

template <typename Block, unsigned count>
inline Repeat<Block, count> Xor(Repeat<Block, count> a, Repeat<Block, count> b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Xor(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> Plus32(Repeat<Block, count> a, Repeat<Block, count> b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Plus32(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> Plus(Repeat<Block, count> a, Repeat<Block, count> b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Plus(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> Minus(Repeat<Block, count> a, Repeat<Block, count> b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Minus(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> LeftShift(Repeat<Block, count> a, int s) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = LeftShift(a.it[i], s);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> RightShift32(Repeat<Block, count> a) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = RightShift32(a.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> Times(Repeat<Block, count> a, Repeat<Block, count> b) {
  Repeat<Block, count> result;
  for (unsigned i = 0; i < count; ++i) {
    result.it[i] = Times(a.it[i], b.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline uint64_t Sum(Repeat<Block, count> a) {
  uint64_t result = 0;
  for (unsigned i = 0; i < count; ++i) {
    result += Sum(a.it[i]);
  }
  return result;
}

template <typename Block, unsigned count>
inline Repeat<Block, count> Negate(Repeat<Block, count> a) {
  Repeat<Block, count> b;
  for (unsigned i = 0; i < count; ++i) {
    b.it[i] = Negate(a.it[i]);
  }
  return b;
}

  
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

  
}
