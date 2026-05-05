#pragma once

#include "types.h"

#include <cstddef>
#include <random>
#include <vector>

class EventGenerator {
public:
  EventGenerator(BenchConfig conf);

  Event emitAddEvent();
  Event emitModifyEvent();
  Event emitCancelEvent();

  Workload generate();

private:
  struct LiveOrder {
    OrderId id;
    Quantity qty;
  };

  BenchConfig config_;
  std::mt19937_64 rng_;

  OrderId nextId_;
  std::vector<LiveOrder> liveOrders_;

  std::size_t pickLiveIndex();
};
