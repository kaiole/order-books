#pragma once

#include "config.h"
#include "order_book/types.h"
#include "workload/event.h"

#include <cstddef>
#include <random>
#include <vector>

class EventGenerator {
public:
  explicit EventGenerator(WorkloadConfig conf);

  Event emitAddEvent();
  Event emitModifyEvent();
  Event emitCancelEvent();

  Workload generate();

private:
  struct LiveOrder {
    OrderId id;
    Quantity qty;
  };

  WorkloadConfig config_;
  std::mt19937_64 rng_;

  std::bernoulli_distribution sideDist_;
  std::uniform_int_distribution<Price> bidOffsetDist_;
  std::uniform_int_distribution<Price> askOffsetDist_;
  std::uniform_int_distribution<Quantity> qtyDist_;
  std::discrete_distribution<int> eventDist_;

  OrderId nextId_;
  std::vector<LiveOrder> liveOrders_;

  std::size_t pickLiveIndex();
};
