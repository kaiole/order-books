#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

inline constexpr std::string_view defaultOutputDir = "artifacts/bench";

enum class Impl : std::uint8_t { Ghost, OrderBook };

struct EventMix {
  double add;
  double modify;
  double cancel;
};

struct TifMix {
  double gtc;
  double ioc;
  double fok;
};

struct WorkloadConfig {
  std::size_t eventCount = 1'000'000;
  std::size_t warmupCount = 100'000;
  std::uint64_t seed = 67;
  EventMix mix{0.6, 0.2, 0.2};
  Price midPrice = 10'000;
  Price maxPriceOffset = 50;
  double crossProbability = 0.0;
  TifMix tifMix{1.0, 0.0, 0.0};
  Quantity minQty = 1;
  Quantity maxQty = 100;
};

struct RunConfig {
  Impl impl = Impl::Ghost;
  std::string outputDir{defaultOutputDir};
  WorkloadConfig workload{};
};
