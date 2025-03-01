#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "crypto_hash.h"
#include "halftime-hash.hpp"
#include "seed.hpp"
#include "log-block-width.hpp"

int crypto_hash(unsigned char *out, const unsigned char *in, unsigned long long inlen) {
  static constexpr int kOutWidth = (crypto_hash_BYTES / 8) - 1;
  uint64_t out_local[kOutWidth + 1];
  static_assert(sizeof(inlen) == sizeof(size_t),
                "Mismatch between size_t (used in HalftimeHash) and unsigned long long "
                "(used in crypto_hash).");
  using Wrapper =
      halftime_hash::FixedWidthBlock<HALFTIME_HASH_LOG_BLOCK_WIDTH>::BlockWrapper;
  static_assert(
      sizeof(seed) / sizeof(seed[0]) >=
          halftime_hash::advanced::GetEntropyBytesNeeded<Wrapper, kOutWidth>(~0ull),
      "Seed too short");

  halftime_hash::advanced::Hash<Wrapper, kOutWidth>(seed, in, inlen, out_local);
  out_local[kOutWidth] = inlen;
  memcpy(out, out_local, sizeof(out_local));
  return 0;
}
