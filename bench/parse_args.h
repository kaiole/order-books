#pragma once

#include <cstddef>
#include <cstdint>

enum class Impl { MapDeque, MapList };

struct BenchConf {
  Impl impl = Impl::MapDeque;
  std::size_t workloadSize = 5'000'000;
  std::size_t warmupSize = 200'000;
  std::uint64_t seed = 67;
};

BenchConf parseArgs(int argc, char** argv);
