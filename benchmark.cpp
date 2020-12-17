#include "halftime-hash.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
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
template <uint64_t Hash(const uint64_t entropy[], const char input[],
                        uint64_t char_length),
          typename... U>
inline decltype(chrono::steady_clock::now() - chrono::steady_clock::now()) TimeMulti(
    unsigned count, U&&... args) {
  auto before = Now();
  dummy += Hash(args...);
  auto after = Now();
  decltype(chrono::steady_clock::now() - chrono::steady_clock::now()) result =
      after - before;
  if (1) {
    unsigned plateau = 0;
    for (unsigned i = count; true; true) {
      auto before = Now();
      for (unsigned j = 0; j < i; ++j) {
        dummy += Hash(args...);
      }
      auto after = Now();
      if (result <= (after - before) / i) {
        // cerr << plateau << "\t" << i << "\t" << result.count() << "\t"
        //      << ((after - before) / i).count() << "\t" << endl;
        ++plateau;
      } else {
        // cerr << plateau << "\t" << i << "\t" << result.count() << "\t"
        //      << ((after - before) / i).count() << "\t" << endl;
        result = (after - before) / i;
        i *= 2;
        plateau = 0;
      }
      if (plateau >= 4) break;
    }
  } else {
    auto before = Now();
    for (unsigned j = 0; j < count; ++j) {
      dummy += Hash(args...);
    }
    auto after = Now();
    result = (after - before) / count;
  }
  return result;
}

template <void Hash(const uint64_t entropy[], const char input[], uint64_t char_length,
                    uint64_t output[])>
inline uint64_t WrapHash(const uint64_t entropy[], const char input[],
                         uint64_t char_length) {
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

inline uint64_t ClhashWrap(const uint64_t entropy[], const char input[],
                           uint64_t char_length) {
  return CLHASHbyte(entropy, input, char_length);
}

int main(int argc, char** argv) {
  auto min_length = (argc > 1) ? To<uint64_t>(argv[1]) : 1;
  auto max_length = (argc > 2) ? To<uint64_t>(argv[2]) : 1000 * 1000;
  auto percent_increment = (argc > 3) ? To<double>(argv[3]) : 1;
  max_length = max(max_length, min_length);
  uint64_t entropy[32000 / sizeof(uint64_t)] = {};
  vector<char> data(max_length, 0);

  cout << "0\tv4<3>\tclhash\n";

  uint64_t loop_count = 4;

  map<uint64_t, pair<double, double>> timings;
  for (uint64_t j = 0; j < loop_count; ++j) {
    for (uint64_t i = min_length; i <= max_length;
         i = max(i * (1 + percent_increment / 100.0), i + 1.0)) {
      auto reps = 50.0 * 1000 * 1000 / (i * sqrt(i) + 1);
      reps = max(reps, 8.0);
      reps = min(1000.0 * 1000, reps);
      auto hh_time =
          TimeMulti<WrapHash<halftime_hash::V4<3>>>(reps, entropy, data.data(), i);
      auto cl_time = TimeMulti<ClhashWrap>(reps, entropy, data.data(), i);
      if (timings.find(i) == timings.end()) {
        timings[i] = {1.0 * i / hh_time.count(), 1.0 * i / cl_time.count()};
      }
      timings[i].first = max(timings[i].first, 1.0 * i / hh_time.count());
      timings[i].second = max(timings[i].second, 1.0 * i / cl_time.count());
    }
  }

  for (auto& j : timings) {
    cout << setprecision(20) << j.first << "\t" << j.second.first << "\t"
         << j.second.second << endl;
  }

  if (dummy == 867 + 5309) cerr << "compiler: no skipping!" << endl;
}
