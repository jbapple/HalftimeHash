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

template <void Hash(const uint64_t entropy[], const char input[], uint64_t char_length,
                    uint64_t output[])>
inline void TestHash(const uint64_t entropy[], char input[], uint64_t char_length,
                     uint64_t location) {
  array<uint64_t, 5> output = {};
  Hash(entropy, input, char_length, &output[0]);
  array<uint64_t, 5> output2 = output;
  assert(output == output2);
  input[location]++;
  Hash(entropy, input, char_length, &output2[0]);
  if (output2 == output) {
    cerr << char_length << ' ' << location << '\n';
    for (unsigned i = 0; i < 5; ++i) {
      cerr << output[i] << ' ';
    }
    cerr << endl;
    abort();
  }
}

template <typename T>
T To(const char* data) {
  istringstream is(data);
  T result;
  is >> result;
  return result;
}

int main(int argc, char** argv) {
  auto min_length = (argc > 1) ? To<uint64_t>(argv[1]) : 1;
  auto max_length = (argc > 2) ? To<uint64_t>(argv[2]) : 161000;
  //  auto percent_increment = (argc > 3) ? To<double>(argv[3]) : 2;
  max_length = max(max_length, min_length);
  uint64_t entropy[64000 / sizeof(uint64_t)] = {};
  for(unsigned i = 0; i < sizeof(entropy)/sizeof(entropy[0]); ++i) {
    entropy[i] = rand();
    entropy[i] = entropy[i] << 32;
    entropy[i] |= rand();
  }
  vector<char> data(max_length, 0);
  for (unsigned i = min_length; i < max_length; ++i) {
    for (unsigned j = 0; j < min_length; ++j) {
      TestHash<halftime_hash::V1<5>>(entropy, data.data(), i, j);
    }
  }
}
