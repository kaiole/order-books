#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

enum class Impl { MapDeque, MapList };

struct BenchConf {
  Impl impl = Impl::MapDeque;
  std::size_t workloadSize = 5'000'000;
  std::size_t warmupSize = 200'000;
  std::uint64_t seed = 67;
};

enum class Operation : std::int8_t { Add = 0, Modify = 1, Cancel = 2 };

struct Event {
  Operation op;
  OrderId id;
  Quantity qty;
  Side side;
  Price price;
};

using Workload = std::vector<Event>;
