// Does not compile with Clang 9, must use 10 or later.

#if !defined(__clang__) || (defined(__clang__) && __clang_major__ >= 10)

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <utility>

using namespace std;

struct Rgen {
  uint64_t xState, yState;  // set to nonzero seed

  Rgen() {
    random_device r;
    xState = r();
    xState = xState << 32;
    xState |= r();
    yState = r();
    yState = xState << 32;
    yState |= r();
  }

  static uint64_t ROTL(uint64_t d, int lrot) {
    return ((d << (lrot)) | (d >> (8 * sizeof(d) - (lrot))));
  }

  uint64_t operator()() {
    uint64_t xp = xState;
    xState = 15241094284759029579u * yState;
    yState = yState - xp;
    yState = ROTL(yState, 27);
    return xp;
  }
};

// uint64_t GenScalar() {
//   static thread_local Rgen r;
//   uint64_t result = 0;
//   switch (r() % 10) {
//     case 0:
//     case 1:
//     case 2:
//     case 3:
//     case 4:
//       result = 1ull << (r() % 5);
//       break;
//     case 5:
//     case 6:
//     case 7:
//       break;
//     default:
//       result = 1ull << (r() % 5);
//       result |= 1ull << (r() % 5);
//   }
//   if (3 == (r() & 3)) result = ~result;
//   if (result > 100 && -result > 100) {
//     cerr << "uhoh " << result << endl << -result << endl;
//   }
//   return result;
// }

uint64_t OldGenScalar() {
  static thread_local Rgen r;
  uint64_t result = 0;
  switch (r() % 4) {
    case 0:
    case 2:
      result = 1ull << (r() % 5);
      break;
    case 1:
      //break;
    default:
      result = 1ull << (r() % 5);
      result |= 1ull << (r() % 5);
  }
  if (3 == (r() & 3)) result = -result;
  if (result > 100 && -result > 100) {
    cerr << "uhoh " << result << endl << -result << endl;
  }
  return result;
}


uint64_t GenScalar() {
  static thread_local Rgen r;
  unsigned x = r();
  if ((x % 40) < 12) return 0;
  switch ((x >> 4) % 10) {
    case 6:
      return 0;
    default:
      return ((x >> 4) % 10);
  }
}

void PrintScalar(uint64_t x) {
  if (x < 10 || -x < 10) {
    cout << " ";
  }
  if (x > -x) {
    cout << "-";
    x = -x;
  } else {
    cout << " ";
  }
  cout << x;
}



template <int height>
struct HeightBased {
  static int WeighScalar(uint64_t y, int n, uint64_t seen[height]) {
    int result = 0;
    auto x = y;
    if (x == 0) return 0;
    if (x > -x) {
      x = -x;
      result += 2;
    }
    int lowest = __builtin_popcountl(x) * 2 - 1;
    for (int i = 0; i < n; ++i) {
      if (seen[i] > -seen[i]) continue;
      lowest = min(lowest, max(0, __builtin_popcountl(x & ~(1 | seen[i])) * 2 - 1));
    }
    result += lowest;
    // cout << "WeighScalar ";
    // PrintScalar(y);
    // cout << " |-> " << result << endl;
    return result;
  }

  static void GenColumn(uint64_t output[height]) {
    for (int i = 0; i < height; ++i) {
      output[i] = GenScalar();
    }
  }

  static void GenMatrix(int length, uint64_t columns[][height]) {
    if (3 == height) return GenMatrix3(length, reinterpret_cast<uint64_t(*)[3]>(columns));
    if (4 == height) {
      for (int i = 0; i < length; ++i) {
        for (int j = 0; j < height; ++j) {
          columns[i][j] = GenScalar();
        }
      }
      return;
    }
    for (int i = 0; i < height; ++i) {
      for (int j = 0; j < height; ++j) {
        columns[i][j] = 0;
      }
      columns[i][i] = 1;
    }
    for (int i = 0; i < height; ++i) {
      columns[height][i] = 1;
    }
    // speculative:
    for (int i = 0; i < height; ++i) {
      columns[height + 1][i] = i + 1;
    }
    // general
    for (int i = height + 2; i < length; ++i) {
      GenColumn(columns[i]);
    }
  }

  // evenness: 2 weight: 11
//  0   0   1   4   1   1   2   2   1
//  1   1   0   0   1   4  17   2   2
//  1   4   1   1   0   0   2   1   2

  // evenness: 2 weight: 10
  //  0   0   1   4   1   1   2   2   1
  //  1   1   0   0   1   4   1   2   2
  //  1   4   1   1   0   0   2   1   2

  static void GenMatrix3(int length, uint64_t columns[][3]) {
    columns[0][0] = columns[1][0] = 0;
    columns[2][1] = columns[3][1] = 0;
    columns[4][2] = columns[5][2] = 0;

    columns[0][1] = columns[1][1] = 1;
    columns[2][2] = columns[3][2] = 1;
    columns[4][0] = columns[5][0] = 1;

    columns[0][2] = columns[2][0] = columns[4][1] = 1;
    columns[1][2] = columns[3][0] = columns[5][1] = 4;//GenScalar();

    columns[6][0] = columns[6][2] = 2;
    columns[7][0] = columns[7][1] = 2;
    columns[8][1] = columns[8][2] = 2;

    columns[6][1] = columns[7][2] = columns[8][0] = 1;

    
    //columns[7][0] = 1;
    //    columns[7][1] = ;
    //    columns[7][2] = -2;
    //GenColumn(columns[6]);
    //GenColumn(columns[7]);
    //GenColumn(columns[8]);
    // columns[7][0] = columns[7][1] = columns[8][0] = 1;
    // columns[7][2] = columns[8][1] = 2;
    // columns[8][2] = 3;
    // columns[7][0] = 2;
  }

  static int OldWeighMatrix(int length, uint64_t columns[][height]) {
    int result = 0;
    for (int i = 0; i < length; ++i) {
      uint64_t seen[height];
      for (int j = 0; j < height; ++j) {
        result += WeighScalar(columns[i][j], j, seen);
        seen[j] = columns[i][j];
      }
    }
    return result;
  }

  static int WeighMatrix(int length, uint64_t columns[][height]) {
    int result = 0;
    for (int i = 0; i < length; ++i) {
      uint64_t seen = (columns[i][0] == 7) ? 8 : columns[i][0];
      for (int j = 1; j < height; ++j) {
        seen |= (columns[i][j] == 7) ? 8 : columns[i][j];
      }
      result += __builtin_popcountl(seen);
    }
    return result;
  }

  static void PrintMatrix(int length, uint64_t columns[][height]) {
    for (int i = 0; i < height; ++i) {
      for (int j = 0; j < length; ++j) {
        PrintScalar(columns[j][i]);
        cout << " ";
      }
      cout << endl;
    }
  }
};

uint64_t Determinant2 (const uint64_t * input[2]) {
  return input[0][0] * input[1][1] - input[0][1] * input[1][0];
}

uint64_t Determinant3(const uint64_t* input[3]) {
  const uint64_t * inner[2];
  uint64_t result = 0;
  inner[0] = &input[1][1];
  inner[1] = &input[2][1];
  result = result + input[0][0] * Determinant2(inner);
  inner[0] = &input[0][1];
  result = result - input[1][0] * Determinant2(inner);
  inner[1] = &input[1][1];
  result = result + input[2][0] * Determinant2(inner);
  return result;
}

uint64_t Determinant4(const uint64_t* input[4]) {
  const uint64_t* inner[3];
  uint64_t result = 0;
  inner[0] = &input[1][1];
  inner[1] = &input[2][1];
  inner[2] = &input[3][1];
  result = result + input[0][0] * Determinant3(inner);
  inner[0] = &input[0][1];
  result = result - input[1][0] * Determinant3(inner);
  inner[1] = &input[1][1];
  result = result + input[2][0] * Determinant3(inner);
  inner[2] = &input[2][1];
  result = result - input[3][0] * Determinant3(inner);
  return result;
}

int MostEvenDeterminant3(int length, const uint64_t input[][3]) {
  const uint64_t * columns[3];
  int result = 0;
  for(int i = 0; i < length; ++i) {
    columns[0] = input[i];
    for(int j = i + 1; j < length; ++j) {
      columns[1] = input[j];
      for(int k = j + 1; k < length; ++k) {
        columns[2] = input[k];
        auto det = Determinant3(columns);
        auto evenness = det ? __builtin_ctzll(det) : 64;
        result = max(result, evenness);
        if (result == 64) {
          return result;
        }
      }
    }
  }
  return result;
}

int MostEvenDeterminant4(int length, const uint64_t input[][4]) {
  const uint64_t* columns[4];
  int result = 0;
  for (int i = 0; i < length; ++i) {
    columns[0] = input[i];
    for (int j = i + 1; j < length; ++j) {
      columns[1] = input[j];
      for (int k = j + 1; k < length; ++k) {
        columns[2] = input[k];
        for (int m = k + 1; m < length; ++m) {
          columns[3] = input[m];
          auto det = Determinant4(columns);
          auto evenness = det ? __builtin_ctzll(det) : 64;
          result = max(result, evenness);
          if (64 == result) return 64;
        }
      }
    }
  }
  return result;
}

/*

evenness: 4 weight: 120
 -1  18  -9  20   2   1 -25   0  -1   1   2   9  -2 -11  -1
  1  -7  -3   0   1   0  -1  -3  -9   8  -9  16   6   6  -1
  3   3  -3  -7   2   4   6   2   1  -5   4   0   1   4   0

evenness: 4 weight: 117
 -1  20   8  -1   0  10  -1   1  -5  -1   1   1   2  -9   0
  2 -13  -2  -9   2   1 -19   1 -10   0   0  -5   6   8  -1
 -1   0   1   4  -3  -9  -1  -3   0 -18  -1   2 -17   4   3

evenness: 1 weight: 25
  2   1   1   2   1   2   1
  1   1 -12  16   2   1   1
  2   1   3   1   0   1   0

evenness: 2 weight: 38
  2   1   0   8  -1   1   1   4  10
  3   0   1   0  18   2   4  -1   5
  9   4   1   1   1   0   9   0   4

evenness: 4 weight: 23
  0   4   1   1   4   2   1   2   0
 10   0   2   0   1   4  16   1   2
  1   1   1   2   0  16   0   8   2

evenness: 3 weight: 28
  1   5   3   1   8   1   1   4  16
  4   1   2   4   1   0   0   4   1
  2   8   0   4   1   2   8   1   2

evenness: 3 weight: 42 - 4*2 = 34
  1  -6  -1  -8   5   1  16   8   3   0
  0   1   1   8   1  16   5   1   2   2
  8   1   1   1   0   1   2   0   4   5

evenness: 3 weight: 42 - 6*2 = 30
 10   4   1   1  -2   0  17   2   4   0
 -1   0   1   0   3   8   4   1   1   1
  1   9   1  17   1   1   0   0  -8  -4

evenness: 3 weight: 28
  1   3   1   0   9   1   0  -8   2   1
  1   8   2   2   1  -2   5   0   2   0
  4   0   1   9   2   0  -1   1  -1   2

*/

void main_loop(){
  // evenness, weight -> max columns
  // 0 -> 5
  // 1 -> 7
  // 2 -> 9
  // 3 -> 12
  // 4, 119 -> 15
  //
  static constexpr int kColumns = 10;
  int target = 0;
  int weightTarget = 0;
  uint64_t m[kColumns][4];
  // GenMatrix(kColumns, m);
  // PrintMatrix(kColumns, m);
  // cout << WeighScalar(33) << endl << WeighScalar(-33) << endl;
  // cout << "MED: " << MostEvenDeterminant(kColumns, m) << endl;
  using Foo = HeightBased<4>;
  int best = 64 + 1;
  int bestWeight = kColumns * 4 * 77;
  vector<int> topWeights(best, bestWeight);
  while (best > target || bestWeight > weightTarget) {
    Foo::GenMatrix(kColumns, m);
    auto here = MostEvenDeterminant4(kColumns, m);
    if (here == 64) continue;
    auto weight = Foo::WeighMatrix(kColumns, m);
    if (topWeights[here] <= weight) continue;
    {
      topWeights[here] = weight;
      cout << "evenness: " << here << " weight: " << weight << endl;
      Foo::PrintMatrix(kColumns, m);
    }
  }
}

int main() {
  main_loop();
}

#else  // !defined(__clang__)

#include <iostream>

using namespace std;

int main(){
  cerr << "Clang 9 can't compile me" << endl;
}
#endif
