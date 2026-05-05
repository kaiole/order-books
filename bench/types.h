#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

enum class Impl { MapDeque, MapList };

struct BenchConfig {
  Impl impl;
  std::size_t eventCount = 1'000'000;
  std::size_t warmupCount = 100'000;
  std::uint64_t seed = 67;
  double addRatio;
  double cancelRatio;
  double modifyRatio;
  Price midPrice = 10'000;
  Price maxPriceOffset = 50;
  Quantity minQty = 1;
  Quantity maxQty = 100;
};

enum class EventType : std::int8_t { Add = 0, Modify = 1, Cancel = 2 };

struct Event {
  EventType eventName;
  OrderId id;
  Side side;
  Price price;
  Quantity qty;
};

using Workload = std::vector<Event>;
