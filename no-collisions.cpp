#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "halftime-hash.hpp"

using namespace std;
using namespace halftime_hash;
using namespace halftime_hash::advanced;

int main(int, char**) {
  map<array<uint64_t,2>, int> seen;

  array<uint64_t, 22> a;

  array<uint64_t, 2> result;

  uint64_t entropy[5000];
  for (unsigned i = 0; i < sizeof(entropy)/sizeof(entropy[0]); ++i) {
    entropy[i] = rand();
    entropy[i] <<= 32;
    entropy[i] |= rand();
  }

  for (int b = 0; b < 1 << 22; ++b) {
    for (int i = 0; i < 22; ++i) {
      a[i] = ((b >> i) & 1) ? 1 : 0;
    }
    V1<2>(entropy, reinterpret_cast<const char*>(&a[0]),
                         22 * sizeof(uint64_t), &result[0]);
    auto found = seen.find(result);
    if (found != seen.end()) {
      cout << hex << found->second << endl
           << b << endl
           << result[0] << ", " << result[1] << endl;
      V1<2>(entropy, reinterpret_cast<const char*>(&a[0]),
                           22 * sizeof(uint64_t), &result[0]);
      auto c = found->second;
      for (int i = 0; i < 22; ++i) {
        a[i] = ((c >> i) & 1) ? 1 : 0;
      }
      V1<2>(entropy, reinterpret_cast<const char*>(&a[0]),
                           22 * sizeof(uint64_t), &result[0]);
      return 1;
    }
    seen[result] = b;
  }
}
