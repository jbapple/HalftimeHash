#include "halftime-hash.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

using namespace std;

#include <immintrin.h>
#include <x86intrin.h>

#include "clhash/clhash.h"

decltype(chrono::steady_clock::now()) Now() {
  _mm_mfence();
  auto result = chrono::steady_clock::now();
  _mm_mfence();
  return result;
}

uint64_t dummy = 0;

template <typename T, typename... U>
decltype(chrono::steady_clock::now() - chrono::steady_clock::now()) Time(T f,
                                                                         U&&... args) {
  auto before = Now();
  dummy += f(args...);
  auto after = Now();
  return after - before;
}
template <typename T, typename... U>
decltype(chrono::steady_clock::now() - chrono::steady_clock::now()) TimeMulti(
    unsigned count, T f, U&&... args) {
  auto before = Now();
  dummy += f(args...);
  auto after = Now();
  decltype(chrono::steady_clock::now() - chrono::steady_clock::now()) result =
      after - before;
  if (1) {
    for (unsigned i = 1; 2 * i < count; i *= 2) {
      auto before = Now();
      for (unsigned j = 0; j < i; ++j) {
        dummy += f(args...);
      }
      auto after = Now();
      result = min(result, (after - before) / i);
    }
  } else {
    auto before = Now();
    for (unsigned j = 0; j < count; ++j) {
      dummy += f(args...);
    }
    auto after = Now();
    result = (after - before) / count;
  }
  return result;
}

template<void Hash(const uint64_t entropy[], const char input[], uint64_t char_length, uint64_t output[])>
uint64_t WrapHash(const uint64_t entropy[], const char input[], uint64_t char_length) {
  uint64_t output[5];
  Hash(entropy, input, char_length, output);
  return output[0];
}

template <typename T>
T To(const char* data) {
  istringstream is(data);
  T result;
  is >> result;
  return result;
}

uint64_t ClhashWrap(const uint64_t entropy[], const char input[], uint64_t char_length) {
  return CLHASHbyte(entropy, input, char_length);
}

template <uint64_t Hash(const uint64_t entropy[], const char input[],
                        uint64_t char_length)>
map<uint64_t, double> Loop(uint64_t loop_count, uint64_t min_length, uint64_t max_length,
                           double percent_increment, const uint64_t* entropy,
                           const char* input) {
  map<uint64_t, double> result;
  for (uint64_t j = 0; j < loop_count; ++j) {
    for (uint64_t i = min_length; i <= max_length;
         i = max(i * (1 + percent_increment / 100.0), i + 1.0)) {
      auto reps = 2000.0 * 1000 * 1000 / (i * sqrt(i) + 1);
      reps = max(reps, 30.0);
      reps = min(1000.0 * 1000, reps);
      auto time = TimeMulti(reps, Hash, entropy, input, i);
      if (result.find(i) == result.end()) {
        result[i] = 1.0 * i / time.count();
      }
      result[i] = max(result[i], 1.0 * i / time.count());
    }
  }
  return result;
}

int main(int argc, char** argv) {
  auto min_length = (argc > 1) ? To<uint64_t>(argv[1]) : 1;
  auto max_length = (argc > 2) ? To<uint64_t>(argv[2]) : 1000 * 1000;
  auto percent_increment = (argc > 3) ? To<double>(argv[3]) : 1;
  max_length = max(max_length, min_length);
  uint64_t entropy[32000 / sizeof(uint64_t)] = {};
  vector<char> data(max_length, 0);

  cout << "0\tv4<3>\tclhash\n";
  const auto hh = Loop<WrapHash<halftime_hash::V4<3>>>(
      3, min_length, max_length, percent_increment, entropy, data.data());
  const auto cl = Loop<ClhashWrap>(30, min_length, max_length, percent_increment, entropy,
                                   data.data());
  auto i = hh.begin();
  for (auto& j : cl) {
    assert(i->first == j.first);
    assert(i != hh.end());
    cout << j.first << "\t" << i->second << "\t" << j.second << endl;
    ++i;
  }
  assert(i == hh.end());
  if (dummy == 867 + 5309) cerr << "compiler: no skipping!" << endl;
}
