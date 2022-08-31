#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "StronglyUniversalStringHashing/include/clhash.h"
#include "halftime-hash.hpp"
#include "umash/umash.h"

using namespace std;

using namespace halftime_hash;
using namespace halftime_hash::advanced;

#if defined(__x86_64)
#include <immintrin.h>
#include <x86intrin.h>

int64_t Now() {
  _mm_mfence();
  return _rdtsc();
  _mm_mfence();
}
using Duration = int64_t;
#elif defined(__ARM_ARCH)
decltype(chrono::steady_clock::now()) Now() {
  _mm_mfence();
  auto result = chrono::steady_clock::now();
  _mm_mfence();
  return result;
}
using Duration = decltype(chrono::steady_clock::now() - chrono::steady_clock::now());
#else
decltype(chrono::steady_clock::now()) Now() {
  auto result = chrono::steady_clock::now();
  return result;
}
using Duration = decltype(chrono::steady_clock::now() - chrono::steady_clock::now());
#endif

uint64_t dummy = 0;


template <typename T, typename... U>
Duration Time(T f, U&&... args) {
  auto before = Now();
  dummy += f(args...);
  auto after = Now();
  return after - before;
}
template <uint64_t Hash(const uint64_t entropy[], const char input[],
                        uint64_t char_length),
          typename... U>
inline Duration TimeMulti(unsigned count, U&&... args) {
  auto before = Now();
  dummy += Hash(args...);
  auto after = Now();
  auto result = after - before;
  if (1) {
    unsigned plateau = 0;
     unsigned i = count;
     while (true) {
       auto before = Now();
       for (unsigned j = 0; j < i; ++j) {
         dummy += Hash(args...);
       }
       auto after = Now();
       if (result <= (after - before) / i) {
         // cerr << plateau << "\t" << i << "\t" << result/*.count()*/ << "\t"
         //      << ((after - before) / i)/*.count()*/ << "\t" << endl;
         ++plateau;
       } else {
         // cerr << plateau << "\t" << i << "\t" << result/*.count()*/ << "\t"
         //      << ((after - before) / i)/*.count()*/ << "\t" << endl;
         result = (after - before) / i;
         i *= 2;
         plateau = 0;
       }
       if (plateau >= 16) break;
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

uint64_t clhashWrap128(const uint64_t *rs, const char *stringword, const size_t length) {
  auto x = CLHASHbyte(rs, reinterpret_cast<const char *>(stringword), length);
  x ^= CLHASHbyte(rs +8, reinterpret_cast<const char *>(stringword), length);
  return x;
}

inline uint64_t umashWrap(const uint64_t params[], const char data[], size_t length) {
  return umash_full(reinterpret_cast<const umash_params*>(params), 42, 0, data, length);
}

uint64_t umash128(const uint64_t *params, const char *data, const size_t length) {
  auto x = umash_fprint(reinterpret_cast<const umash_params *>(params), 42, data, length);
  return x.hash[0] ^ x.hash[1];
}

int main(int argc, char** argv) {
  auto min_length = (argc > 1) ? To<uint64_t>(argv[1]) : 1;
  auto max_length = (argc > 2) ? To<uint64_t>(argv[2]) : 4000;
  auto percent_increment = (argc > 3) ? To<double>(argv[3]) : 2;
  max_length = max(max_length, min_length);
  uint64_t entropy[32000 / sizeof(uint64_t)] = {};

  umash_params* umash_seeds = reinterpret_cast<umash_params*>(entropy);
  while (true) {
    for (int i = 0; i < static_cast<int>(sizeof(*umash_seeds) / sizeof(uint64_t)); ++i) {
      const uint64_t seed = rand() | ((uint64_t)(rand()) << 32);
      reinterpret_cast<uint64_t*>(umash_seeds)[i] = seed;
    }
    if (umash_params_prepare(umash_seeds)) break;
  }

  vector<char> data(max_length, 0);

  cout << "0 \t best_hh";
  for (int i : {4, 3}) {
    for (int j : {2,3,4,5}) {
      cout << "\t"
           << "Halftime" << (j * 8) << "v" << i;
    }
  }
  cout << "\t clhash \t UMASH \t UMASH128";
  cout << endl;

  uint64_t loop_count = 4;

  map<uint64_t, array<double, 20>> timings;
  for (uint64_t j = 0; j < loop_count; ++j) {
    for (uint64_t i = min_length; i <= max_length;
         i = max(i * (1 + percent_increment / 100.0), i + 1.0)) {
      auto reps = 50.0 * 1000 * 1000 / (i * sqrt(i) + 1);
      reps = max(reps, 8.0);
      reps = min(1000.0 * 1000, reps);
      Duration hh_time[8] = {
          // TimeMulti<clhashWrap128>(reps, entropy, data.data(), i),
          // TimeMulti<umash128>(reps, entropy, data.data(), i)
          TimeMulti<WrapHash<V4<2>>>(reps, entropy, data.data(), i),
          TimeMulti<WrapHash<V4<3>>>(reps, entropy, data.data(), i),
          TimeMulti<WrapHash<V4<4>>>(reps, entropy, data.data(), i),
          TimeMulti<WrapHash<V4<5>>>(reps, entropy, data.data(), i),
          TimeMulti<WrapHash<V3<2>>>(reps, entropy, data.data(), i),
          TimeMulti<WrapHash<V3<3>>>(reps, entropy, data.data(), i),
          TimeMulti<WrapHash<V3<4>>>(reps, entropy, data.data(), i),
          TimeMulti<WrapHash<V3<5>>>(reps, entropy, data.data(), i),

          // TimeMulti<WrapHash< Hash<RepeatWrapper<BlockWrapper512, 2>, 6, 2,
          // encoded_dimension,
          //     out_width>(entropy, char_input, length, output);V4<5>>>(reps, entropy,
          //     data.data(), i),

          // TimeMulti<WrapHash<V3<2>>>(reps, entropy, data.data(), i),
          // TimeMulti<WrapHash<V3<3>>>(reps, entropy, data.data(), i),
          // TimeMulti<WrapHash<V3<4>>>(reps, entropy, data.data(), i),
          // TimeMulti<WrapHash<V3<5>>>(reps, entropy, data.data(), i),

          // TimeMulti<WrapHash<V2<2>>>(reps, entropy, data.data(), i),
          // TimeMulti<WrapHash<V2<3>>>(reps, entropy, data.data(), i),
          // TimeMulti<WrapHash<V2<4>>>(reps, entropy, data.data(), i),
          // TimeMulti<WrapHash<V2<5>>>(reps, entropy, data.data(), i),

          // TimeMulti<WrapHash<V1<2>>>(reps, entropy, data.data(), i),
          // TimeMulti<WrapHash<V1<3>>>(reps, entropy, data.data(), i),
          // TimeMulti<WrapHash<V1<4>>>(reps, entropy, data.data(), i),
          // TimeMulti<WrapHash<V1<5>>>(reps, entropy, data.data(), i),
      };

      auto cl_time = TimeMulti<ClhashWrap>(reps, entropy, data.data(), i);
      // auto cl_time128 = TimeMulti<clhashWrap128>(reps, entropy, data.data(), i);
      auto um_time = TimeMulti<umashWrap>(reps, entropy, data.data(), i);
      auto um_time128 = TimeMulti<umash128>(reps, entropy, data.data(), i);

      if (timings.find(i) == timings.end()) {
        timings[i] = {};
      }
      int k = 0;
      for (; k < 8; ++k) {
        timings[i][k] = max(timings[i][k], 1.0 * i / hh_time[k] /*.count()*/);
      }

      timings[i][k + 0] = max(timings[i][k + 0], 1.0 * i / cl_time/*.count()*/);
      // timings[i][k + 1] = max(timings[i][k + 1], 1.0 * i / cl_time128.count());
      timings[i][k + 1] = max(timings[i][k + 1], 1.0 * i / um_time/*.count()*/);
      timings[i][k + 2] = max(timings[i][k + 2], 1.0 * i / um_time128/*.count()*/);
    }
  }

  for (auto& j : timings) {
    cout << setprecision(8) << j.first;
    auto best_hh = j.second[0];
    for (int i = 1; i < 8; ++i) {
      best_hh = max(best_hh, j.second[i]);
    }
    cout << "\t" << best_hh;
    for (int i = 0; i < 8; ++i) {
      cout << "\t" << j.second[i];
    }
    cout << "\t" << j.second[4];
    cout << "\t" << j.second[5];
    cout << "\t" << j.second[6];
    cout << endl;
  }

  if (dummy == 867 + 5309) cerr << "compiler: no skipping!" << endl;
}
