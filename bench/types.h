#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class Impl { MapDeque, MapList };

struct EventMix {
  double add;
  double modify;
  double cancel;
};

struct WorkloadConfig {
  std::size_t eventCount = 1'000'000;
  std::size_t warmupCount = 100'000;
  std::uint64_t seed = 67;
  EventMix mix{0.6, 0.2, 0.2};
  Price midPrice = 10'000;
  Price maxPriceOffset = 50;
  Quantity minQty = 1;
  Quantity maxQty = 100;
};

struct RunConfig {
  Impl impl = Impl::MapDeque;
  std::string scenarioName = "default";
  std::string outputDir = "results";
  WorkloadConfig workload{};
};

enum class EventType : std::uint8_t { Add = 0, Modify = 1, Cancel = 2 };

struct Event {
  EventType eventType;
  OrderId id;
  Side side;
  Price price;
  Quantity qty;
};

using Workload = std::vector<Event>;

struct LatencyRecord {
  std::int64_t latencyNs;
  std::uint8_t eventType;
  std::uint8_t pad[7]{};
};

static_assert(sizeof(LatencyRecord) == 16);

struct RawHeader {
  char magic[8];
  std::uint32_t version;
  std::uint32_t recordSize;
  std::uint64_t recordCount;
};

static_assert(sizeof(RawHeader) == 24);
