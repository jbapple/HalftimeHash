#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>

#include "halftime-hash.hpp"

using namespace std;

int main() {
  array<uint64_t, halftime_hash::kEntropyBytesNeeded / sizeof(uint64_t)> entropy;
  generate(entropy.begin(), entropy.end(), mt19937_64{});
  string input;
  while (cin >> input) {
    cout << hex
         << halftime_hash::HalftimeHashStyle512(entropy.data(), input.data(),
                                                input.size())
         << endl;
  }
}
