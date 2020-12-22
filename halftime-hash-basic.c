#include <immintrin.h>
#include <stdint.h>
#include <string.h>

typedef __m512i Block;

void DistributeRaw(Block* ioslot, const Block** iter, unsigned label, unsigned rest) {
  for (unsigned i = 0; i < 3; ++i) {
    if ((rest & (1 << i)) != 0) {
      ioslot[i] = _mm512_xor_epi32(ioslot[i], (*iter)[label]);
    }
  }
}

void Distribute3(Block* ioslot, const Block** iter, unsigned a, unsigned b, unsigned c) {
  DistributeRaw(ioslot, iter, 0, a);
  DistributeRaw(ioslot, iter, 1, b);
  DistributeRaw(ioslot, iter, 2, c);
  *iter += 3;
}

void Encode(Block io[9 * 3]) {
  const Block* iter = &io[0];
  io[7 * 3 + 0] = io[8 * 3 + 0] = iter[0];
  io[7 * 3 + 1] = io[8 * 3 + 1] = iter[1];
  io[7 * 3 + 2] = io[8 * 3 + 2] = iter[2];
  iter += 3;

  const unsigned x = 0x001, y = 0x010, z = 0x100;

  while (iter != io + 9 * 3) {
    Distribute3(&io[7 * 3], &iter, x, y, z);
  }

  iter = &io[3];

  Distribute3(&io[8 * 3], &iter, z,    x^z,     y);
  Distribute3(&io[8 * 3], &iter, x^z, x^y^z,  y^z);
  Distribute3(&io[8 * 3], &iter, y,    y^z,   x^z);
  Distribute3(&io[8 * 3], &iter, x^y,   z,      x);
  Distribute3(&io[8 * 3], &iter, y^z,  x^y, x^y^z);
  Distribute3(&io[8 * 3], &iter, x^y^z, z,    x^y);
}


Block Mix(Block input, Block entropy) {
  Block output = _mm512_add_epi32(entropy, input);
  Block twin = _mm512_srli_epi64(output, 32);
  output = _mm512_mul_epu32(output, twin);
  return output;
}

inline void Combine(const Block input[9], Block output[3]) {
  // column 0
  output[1] = input[0];
  output[2] = input[0];

  // column 1
  output[1] = _mm512_add_epi64(output[1], input[1]);
  output[2] = _mm512_add_epi64(output[2], _mm512_slli_epi64(input[1], 2));

  output[0] = input[2];
  output[2] = _mm512_add_epi64(output[2], input[2]);

  output[0] = _mm512_add_epi64(output[0], _mm512_slli_epi64(input[3], 2));
  output[2] = _mm512_add_epi64(output[2], input[3]);

  output[0] = _mm512_add_epi64(output[0], input[4]);
  output[1] = _mm512_add_epi64(output[1], input[4]);

  output[0] = _mm512_add_epi64(output[0], input[5]);
  output[1] = _mm512_add_epi64(output[1], _mm512_slli_epi64(input[5], 2));

  output[0] = _mm512_add_epi64(output[0], _mm512_slli_epi64(input[6], 1));
  output[1] = _mm512_add_epi64(output[1], input[6]);
  output[2] = _mm512_add_epi64(output[2], _mm512_slli_epi64(input[6], 1));

  output[0] = _mm512_add_epi64(output[0], _mm512_slli_epi64(input[7], 1));
  output[1] = _mm512_add_epi64(output[1], _mm512_slli_epi64(input[7], 1));
  output[2] = _mm512_add_epi64(output[2], input[7]);

  // column 8
  output[0] = _mm512_add_epi64(output[0], input[8]);
  output[1] = _mm512_add_epi64(output[1], _mm512_slli_epi64(input[8], 1));
  output[2] = _mm512_add_epi64(output[2], _mm512_slli_epi64(input[8], 1));
}


Block MixOne(Block input, uint64_t entropy) {
  return Mix(input, _mm512_set1_epi64(entropy));
}

const unsigned fanout = 8;
const unsigned out_width = 3;
const unsigned dimension = 7;
const unsigned in_width = 3;
const unsigned encoded_dimension = 9;

void EhcUpperLayer(const Block input[fanout][out_width],
                   const uint64_t entropy[out_width * (fanout - 1)],
                   Block output[out_width]) {
  for (unsigned i = 0; i < out_width; ++i) {
    output[i] = input[0][i];
    for (unsigned j = 1; j < fanout; ++j) {
      output[i] = _mm512_add_epi64(
          output[i], MixOne(input[j][i], entropy[(fanout - 1) * i + j - 1]));
    }
  }
}

void Load(const char input[dimension * in_width * sizeof(Block)],
          Block output[dimension * in_width]) {
  for (unsigned i = 0; i < dimension * in_width; ++i) {
    output[i] = _mm512_loadu_si512(&input[i * sizeof(Block)]);
  }
}

void HashEncoded(const Block input[encoded_dimension * in_width],
          const uint64_t entropy[encoded_dimension * in_width],
          Block output[encoded_dimension]) {
  const Block * a = &input[0];
  const uint64_t * b = &entropy[0];
  for (unsigned i = 0; i < encoded_dimension; ++i) {
    output[i] = *a++;
  }
  for (unsigned j = 1; j < in_width; ++j) {
    for (unsigned i = 0; i < encoded_dimension; ++i) {
      output[i] = _mm512_add_epi64(output[i], MixOne(*a++, *b++));
    }
  }
}

void EhcBaseLayer(const char input[dimension * in_width * sizeof(Block)],
                  const uint64_t raw_entropy[encoded_dimension * in_width],
                  Block output[out_width]) {
  Block scratch[encoded_dimension * in_width];
  Block tmpout[encoded_dimension];
  Load(input, scratch);
  Encode(scratch);
  HashEncoded(scratch, raw_entropy, tmpout);
  Combine(tmpout, output);
}

void DfsTreeHash(const char* data, size_t block_group_length,
                 Block stack[][fanout][out_width], unsigned stack_lengths[],
                 const uint64_t* entropy) {
  for (size_t k = 0; k < block_group_length; ++k) {
    int i = 0;
    while (stack_lengths[i] == fanout) ++i;
    for (int j = i - 1; j >= 0; --j) {
      EhcUpperLayer(stack[j],
                    &entropy[encoded_dimension * in_width + (fanout - 1) * out_width * j],
                    stack[j + 1][stack_lengths[j + 1]]);
      stack_lengths[j] = 0;
      stack_lengths[j + 1] += 1;
    }

    EhcBaseLayer(&data[k * dimension * in_width * sizeof(Block)], &entropy[0],
                 stack[0][stack_lengths[0]]);
    stack_lengths[0] += 1;
  }
}

typedef struct unnamed_bg {
  const uint64_t* seeds;
  Block accum[3];
} BlockGreedy;

void InsertMany(BlockGreedy * that, const Block x[out_width]) {
  for (unsigned i = 0; i < out_width; ++i) {
    that->accum[i] =
        _mm512_add_epi64(that->accum[i], Mix(x[i], _mm512_loadu_si512(that->seeds)));
    that->seeds += sizeof(Block) / sizeof(uint64_t);
  }
}

void InsertOne(BlockGreedy * that, Block x) {
  for (unsigned i = 0; i < out_width; ++i) {
    that->accum[i] = _mm512_add_epi64(
        that->accum[i],
        Mix(x, _mm512_loadu_si512(&that->seeds[i * sizeof(Block) / sizeof(uint64_t)])));
  }
  // Toeplitz
  that->seeds += sizeof(Block) / sizeof(uint64_t);
}

void HashBlockGreedy(const BlockGreedy* that, uint64_t output[out_width]) {
  for (unsigned i = 0; i < out_width; ++i) {
    output[i] = _mm512_reduce_add_epi64(that->accum[i]);
  }
}

void DfsGreedyFinalizer(const Block stack[][fanout][out_width], const unsigned stack_lengths[],
                        const char* char_input, size_t char_length,
                        const uint64_t* entropy, uint64_t output[out_width]) {
  BlockGreedy b;
  b.seeds =entropy;
  b.accum[0] = b.accum[1] = b.accum[2] = _mm512_set1_epi64(0);
  for (int j = 0; stack_lengths[j] > 0; ++j) {
    for (unsigned k = 0; k < stack_lengths[j]; k += 1) {
      InsertMany(&b, stack[j][k]);
    }
  }

  size_t i = 0;
  for (; i + sizeof(Block) <= char_length; i += sizeof(Block)) {
    InsertOne(&b, _mm512_loadu_si512(&char_input[i]));
  }

  Block extra = {};
  memcpy(&extra, &char_input[i], char_length - i);
  InsertOne(&b, extra);

  HashBlockGreedy(&b, output);
}

// evenness: 2 weight: 10
//  0   0   1   4   1   1   2   2   1
//  1   1   0   0   1   4   1   2   2
//  1   4   1   1   0   0   2   1   2


void Hash(const uint64_t* entropy, const char* char_input, size_t length,
          uint64_t output[out_width]) {
#define kMaxStackSize 9

  Block stack[kMaxStackSize][fanout][out_width];
  unsigned stack_lengths[kMaxStackSize] = {};
  size_t wide_length = length / sizeof(Block) / (dimension * in_width);

  DfsTreeHash(char_input, wide_length, stack, stack_lengths, entropy);
  entropy +=
      encoded_dimension * in_width + out_width * (fanout - 1) * kMaxStackSize;

  unsigned used_chars = wide_length * sizeof(Block) * (dimension * in_width);
  char_input += used_chars;

  DfsGreedyFinalizer(stack, stack_lengths, char_input, length - used_chars, entropy,
                     output);
}
