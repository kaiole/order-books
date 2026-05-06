#include "benchmark/benchmark.h"

#include <chrono>
#include <cstddef>
#include <iostream>

int main() {
  constexpr std::size_t n = 10'000'000;

  for (std::size_t i{}; i < 1'000'000; ++i) {
    auto t = std::chrono::steady_clock::now();
    benchmark::DoNotOptimize(t);
  }

  auto t0 = std::chrono::steady_clock::now();
  for (std::size_t i{}; i < n; ++i) {
    auto t = std::chrono::steady_clock::now();
    benchmark::DoNotOptimize(t);
  }
  auto t1 = std::chrono::steady_clock::now();

  auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

  auto nsPerOp = static_cast<double>(elapsed) / static_cast<double>(n);

  std::cout << nsPerOp << " ns per op\n";

  return 0;
}
