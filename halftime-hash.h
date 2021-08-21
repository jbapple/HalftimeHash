#include <stdint.h>
#include <stdlib.h>

extern size_t for_c_kEntropyBytesNeeded;

uint64_t for_c_HalftimeHashStyle512(
    const uint64_t entropy[for_c_kEntropyBytesNeeded / sizeof(uint64_t)],
    const char input[], size_t length);

uint64_t for_c_HalftimeHashStyle256(
    const uint64_t entropy[for_c_kEntropyBytesNeeded / sizeof(uint64_t)],
    const char input[], size_t length);

uint64_t for_c_HalftimeHashStyle128(
    const uint64_t entropy[for_c_kEntropyBytesNeeded / sizeof(uint64_t)],
    const char input[], size_t length);

uint64_t for_c_HalftimeHashStyle64(
    const uint64_t entropy[for_c_kEntropyBytesNeeded / sizeof(uint64_t)],
    const char input[], size_t length);
