#pragma once

#include "config.h"
#include "ghost_book.h"
#include "order_book/types.h"
#include "workload/event.h"

#include <cstddef>
#include <random>
#include <unordered_map>
#include <vector>

class EventGenerator {
public:
  explicit EventGenerator(WorkloadConfig conf);

  Event emitAddEvent(bool forceRest = false);
  Event emitModifyEvent();
  Event emitCancelEvent();

  std::size_t prefillCount() const;
  Workload generate();

private:
  WorkloadConfig config_;
  std::mt19937_64 rng_;

  std::bernoulli_distribution sideDist_;
  std::uniform_int_distribution<Price> bidOffsetDist_;
  std::uniform_int_distribution<Price> askOffsetDist_;
  std::uniform_int_distribution<Quantity> qtyDist_;
  std::discrete_distribution<int> eventDist_;
  std::bernoulli_distribution crossDist_;
  std::discrete_distribution<int> tifDist_;

  OrderId nextId_;

  struct LiveOrder {
    OrderId id;
    Quantity qty;
  };
  std::vector<LiveOrder> liveOrders_;
  std::unordered_map<OrderId, std::size_t> liveIndex_;

  GhostBook ghost_;

  void removeLiveOrder(std::size_t index);
  void applyFill(OrderId passiveId, Quantity filled);

  std::size_t pickLiveIndex();
};
