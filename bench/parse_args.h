#pragma once

#include <cstddef>
#include <cstdint>

enum class Impl { MapDeque, MapList };

struct Args {
  Impl impl = Impl::MapDeque;
  std::size_t workloadSize = 1'000'000;
  std::size_t warmupSize = 100'000;
  std::uint64_t seed = 67;
};

Args parseArgs(int argc, char** argv);
