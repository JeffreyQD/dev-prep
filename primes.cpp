#include <array>
#include <iostream>
#include <type_traits>
#include <variant>

// * Returns the sorted primes not greater than `N` at compile time.
template <size_t N, bool count = false>
constexpr auto primes() {
  if constexpr (count) {
    int cnt = 0;
    bool mask[N + 1] = {false, false};
    for (int i = 2; i <= N; ++i) {
      mask[i] = true;
    }

    for (int i = 2; i <= N; ++i) {
      if (mask[i]) {
        ++cnt;
        for (int j = i << 1; j <= N; j += i) {
          mask[j] = false;
        }
      }
    }

    return cnt;
  } else {
    int idx = 0;
    constexpr size_t n = primes<N, true>();
    bool mask[N + 1] = {false, false};
    for (int i = 2; i <= N; ++i) {
      mask[i] = true;
    }

    std::array<int, n> data{};
    for (int i = 2; i <= N; ++i) {
      if (mask[i]) {
        data[idx++] = i;
        for (int j = i << 1; j <= N; j += i) {
          mask[j] = false;
        }
      }
    }

    return data;
  }
}

constexpr size_t N = 100000;
constexpr auto data = primes<N>();

int main() {
  for (int x : data) {
    std::cout << x << " ";
  }
  std::cout << "\n";
}
