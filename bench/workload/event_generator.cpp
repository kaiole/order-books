#include "event_generator.h"

#include "order_book/types.h"
#include "types.h"

#include <cstddef>
#include <random>
#include <stdexcept>

namespace {

constexpr double bidProbability = 0.5;

} // namespace

EventGenerator::EventGenerator(BenchConfig conf)
    : config_{conf}, rng_{conf.seed}, nextId_{0} {
  // Reserving worst case (all adds) is fine since this part won't negatively
  // impact benchmark results
  liveOrders_.reserve(conf.eventCount + conf.warmupCount);
}

Event EventGenerator::emitAddEvent() {
  std::bernoulli_distribution sideDist{bidProbability};
  std::uniform_int_distribution<Price> bidOffsetDist{1, config_.maxPriceOffset};
  std::uniform_int_distribution<Price> askOffsetDist{0, config_.maxPriceOffset};
  std::uniform_int_distribution<Quantity> qtyDist{config_.minQty,
                                                  config_.maxQty};

  Side side = sideDist(rng_) ? Side::Bid : Side::Ask;
  Price price = side == Side::Bid ? config_.midPrice - bidOffsetDist(rng_)
                                  : config_.midPrice + askOffsetDist(rng_);
  Quantity qty = qtyDist(rng_);

  Event event{.eventName = EventType::Add,
              .id = ++nextId_,
              .side = side,
              .price = price,
              .qty = qty};

  liveOrders_.push_back(LiveOrder{.id = nextId_, .qty = qty});

  return event;
}

Event EventGenerator::emitModifyEvent() {
  auto liveIndex = pickLiveIndex();
  auto& liveOrder = liveOrders_[liveIndex];

  // Orderbook is reduce-only. Can't reduce 1, cancel instead.
  if (liveOrder.qty <= 1) {
    return emitCancelEvent();
  }

  std::uniform_int_distribution<Quantity> newQtyDist{1, liveOrder.qty - 1};
  Quantity newQty = newQtyDist(rng_);

  liveOrder.qty = newQty;

  return Event{.eventName = EventType::Modify,
               .id = liveOrder.id,
               .side = Side{},
               .price = Price{},
               .qty = newQty};
}

Event EventGenerator::emitCancelEvent() {
  auto liveIndex = pickLiveIndex();
  auto& liveOrder = liveOrders_[liveIndex];

  Event event = Event{.eventName = EventType::Cancel,
                      .id = liveOrder.id,
                      .side = Side{},
                      .price = Price{},
                      .qty = {}};

  liveOrder = liveOrders_.back();
  liveOrders_.pop_back();

  return event;
}

Workload EventGenerator::generate() {
  Workload workload;
  const auto totalEvents = config_.eventCount + config_.warmupCount;
  workload.reserve(totalEvents);

  std::discrete_distribution<int> eventDist{
      config_.addRatio, config_.modifyRatio, config_.cancelRatio};

  for (std::size_t i{}; i < totalEvents; ++i) {
    auto eventName = static_cast<EventType>(eventDist(rng_));

    if (eventName != EventType::Add && liveOrders_.empty()) {
      eventName = EventType::Add;
    }

    switch (eventName) {
    case EventType::Add:
      workload.push_back(emitAddEvent());
      break;

    case EventType::Modify:
      workload.push_back(emitModifyEvent());
      break;

    case EventType::Cancel:
      workload.push_back(emitCancelEvent());
      break;
    }
  }

  return workload;
}

std::size_t EventGenerator::pickLiveIndex() {
  if (liveOrders_.empty()) {
    throw std::runtime_error("no live orders");
  }

  auto liveOrderCount = liveOrders_.size();

  std::uniform_int_distribution<std::size_t> indexDist{0, liveOrderCount - 1};

  return indexDist(rng_);
}
