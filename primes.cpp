#include <array>
#include <iostream>
#include <type_traits>
#include <variant>

// * Returns the sorted primes not greater than `N` at compile time.
template <size_t N, size_t n = N>
constexpr auto primes()
    -> std::conditional_t<n == N, size_t, std::array<int, n>> {
  int idx = 0;

  // Use std::monostate to represent nothing.
  std::conditional_t<n == N, std::monostate, std::array<int, n>> data;

  bool mask[N + 1] = {false, false};
  for (int i = 2; i <= N; ++i) {
    mask[i] = true;
  }

  for (int i = 2; i <= N; ++i) {
    if (mask[i]) {
      if constexpr (n == N) {
        ++idx;
      } else {
        data[idx++] = i;
      }

      for (int j = i << 1; j <= N; j += i) {
        mask[j] = false;
      }
    }
  }

  if constexpr (n == N) {
    return idx;
  } else {
    return data;
  }
}

constexpr size_t N = 100000;
constexpr size_t n = primes<N, N>();
constexpr auto data = primes<N, n>();

int main() {
  for (int x : data) {
    std::cout << x << " ";
  }
  std::cout << "\n";
}
