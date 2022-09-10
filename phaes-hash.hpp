#include <immintrin.h>

/*
void AES_CTR_encrypt(const unsigned char *in, unsigned char *out,
                     const unsigned char ivec[8], const unsigned char nonce[4],
                     unsigned long length, const unsigned char *key,
                     int number_of_rounds) {
  __m128i ctr_block, tmp, ONE, BSWAP_EPI64;
  int i, j;
  if (length % 16)
    length = length / 16 + 1;
  else
    length /= 16;
  ONE = _mm_set_epi32(0, 1, 0, 0);
  BSWAP_EPI64 = _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
  ctr_block = _mm_insert_epi64(ctr_block, *(long long *)ivec, 1);
  ctr_block = _mm_insert_epi32(ctr_block, *(long *)nonce, 1);
  ctr_block = _mm_srli_si128(ctr_block, 4);
  ctr_block = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
  ctr_block = _mm_add_epi64(ctr_block, ONE);
  for (i = 0; i < length; i++) {
    tmp = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
    ctr_block = _mm_add_epi64(ctr_block, ONE);
    tmp = _mm_xor_si128(tmp, ((__m128i *)key)[0]);
    for (j = 1; j < number_of_rounds; j++) {
      tmp = _mm_aesenc_si128(tmp, ((__m128i *)key)[j]);
    };
    tmp = _mm_aesenclast_si128(tmp, ((__m128i *)key)[j]);
    tmp = _mm_xor_si128(tmp, _mm_loadu_si128(&((__m128i *)in)[i]));
    _mm_storeu_si128(&((__m128i *)out)[i], tmp);
  }
}
*/

#include <immintrin.h>

constexpr int number_of_rounds = 0;

/*__m512i phaes_hash(const __m512i* input, size_t n, __m512i phkey0, __m512i phkey1,
                   __m512i phkey2, __m512i phkey0, __m512i phkey0, __m512i phkey5,
                   __m512i phkey6, __m512i phkey0, __m512i phkey0, __m512i phkey9,
                   __m512i phkey10, __m512i phkey0, __m512i phkey0, __m512i phkey13,
                   __m512i phkey14, __m512i phkey0, __m512i phkey0, __m512i phkey17,
                   __m512i phkey18, __m512i phkey0, __m512i phkey0, __m512i phkey21,
                   __m512i phkey22, __m512i phkey0, __m512i phkey0, __m512i aeskey0,
                   __m512i aeskey0, __m512i aeskey0, __m512i aeskey0, __m512i aeskey0,
                   __m512i aeskey0, __m512i aeskey0, __m512i aeskey0, ) {
*/



template <int kRounds, int kHi = 28>
__m512i phaes_hash(const __m512i* input, size_t n, const __m512i phkey[kHi - kRounds],
                   const __m512i aeskey[kRounds]) {
  __m512i accum1 = {};
  __m512i ctr_base = {0, 1, 2, 3, 4, 5, 6, 7};
  for (size_t i = 0; i < n; i += kHi - kRounds) {
    auto accum0 = input[i];
    __m512i ctr = _mm512_set1_epi64(i);
    ctr = _mm512_add_epi64(ctr, ctr_base);
    _mm512_aesenc_epi128(ctr, aeskey[0]);
#pragma GCC unroll 32
    for (int j = 1; j < kHi - kRounds; ++j) {
      auto tomult = input[i + j];
      tomult = _mm512_xor_si512(tomult, phkey[j - 1]);
      tomult = _mm512_clmulepi64_epi128(tomult, tomult, 1);
      accum0 = _mm512_xor_si512(accum0, tomult);
    }
    for (int j = 0; j < kRounds - 1; ++j) ctr = _mm512_aesenc_epi128(ctr, aeskey[j]);
    ctr = _mm512_aesenclast_epi128(ctr, aeskey[kRounds - 1]);
    ctr = _mm512_xor_si512(ctr, accum0);
    ctr = _mm512_clmulepi64_epi128(ctr, ctr, 1);
    accum1 = _mm512_xor_si512(accum1, ctr);
    accum1 = _mm512_xor_si512(accum1, accum0);
  }
  return accum1;
}


#include <immintrin.h>

constexpr int kRounds = 10;
constexpr int hi = 23;

__m256i phaes_hash_256(const __m256i* input, size_t n, const __m256i phkey[hi - kRounds],
                       const __m128i aeskey[kRounds]) {
  __m256i accum1 = {};
  __m256i ctr_base = {0, 1, 2, 3};
  for (size_t i = 0; i < n; i += hi - kRounds) {
    auto accum0 = input[i];
    __m256i ctr = _mm256_set1_epi64x(i);
    ctr = _mm256_add_epi64(ctr, ctr_base);
    _mm256_aesenc_epi128(ctr, _mm256_set_m128i(aeskey[0], aeskey[0]));
#pragma GCC unroll 32
    for (int j = 1; j < hi - kRounds; ++j) {
      auto tomult = input[i + j];
      tomult = _mm256_xor_si256(tomult, phkey[j - 1]);
      tomult = _mm256_clmulepi64_epi128(tomult, tomult, 1);
      accum0 = _mm256_xor_si256(accum0, tomult);
    }
    for (int j = 0; j < kRounds - 1; ++j) {
      ctr = _mm256_aesenc_epi128(ctr, _mm256_set_m128i(aeskey[j], aeskey[j]));
    }
    ctr = _mm256_aesenclast_epi128(
        ctr, _mm256_set_m128i(aeskey[kRounds - 1], aeskey[kRounds - 1]));
    ctr = _mm256_xor_si256(ctr, accum0);
    ctr = _mm256_clmulepi64_epi128(ctr, ctr, 1);
    accum1 = _mm256_xor_si256(accum1, ctr);
    accum1 = _mm256_xor_si256(accum1, accum0);
  }
  return accum1;
}


template <int hi = 16>
__m512i ph_single(const __m512i* input, size_t n, const __m512i phkey) {
  __m512i accum[hi] = {};
  for (size_t i = 0; i < n; i += hi) {
    __m512i tomult[hi];
 #pragma GCC unroll 32
   for (int j = 0; j < hi; ++j) {
      tomult[j] = input[i + j];
      tomult[j] = _mm512_xor_si512(tomult[j], phkey);
      tomult[j] = _mm512_clmulepi64_epi128(tomult[j], tomult[j], 1);
      accum[j] = _mm512_xor_si512(accum[j], tomult[j]);
    }
  }
  auto result = accum[0];
  for (int i = 1 ; i < hi; ++i) {
      result ^= accum[i];
  }
  return result;
}
