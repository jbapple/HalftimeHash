#include "halftime-hash.hpp"

using namespace halftime_hash;

size_t for_c_kEntropyBytesNeeded = kEntropyBytesNeeded;

extern "C" uint64_t for_c_HalftimeHashStyle512(
    const uint64_t entropy[kEntropyBytesNeeded / sizeof(uint64_t)], const char input[],
    size_t length) {
  return HalftimeHashStyle512(entropy, input, length);
}

extern "C" uint64_t for_c_HalftimeHashStyle256(
    const uint64_t entropy[kEntropyBytesNeeded / sizeof(uint64_t)], const char input[],
    size_t length) {
  return HalftimeHashStyle256(entropy, input, length);
}


extern "C" uint64_t for_c_HalftimeHashStyle128(
    const uint64_t entropy[kEntropyBytesNeeded / sizeof(uint64_t)], const char input[],
    size_t length) {
  return HalftimeHashStyle128(entropy, input, length);
}

extern "C" uint64_t for_c_HalftimeHashStyle64(
    const uint64_t entropy[kEntropyBytesNeeded / sizeof(uint64_t)], const char input[],
    size_t length) {
  return HalftimeHashStyle64(entropy, input, length);
}
