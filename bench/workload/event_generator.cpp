#include "event_generator.h"

#include "order_book/types.h"
#include "workload/event.h"

#include <cstddef>
#include <stdexcept>

namespace {

constexpr double bidProbability = 0.5;

} // namespace

EventGenerator::EventGenerator(WorkloadConfig conf)
    : config_{conf}, rng_{conf.seed}, sideDist_{bidProbability},
      bidOffsetDist_{1, conf.maxPriceOffset},
      askOffsetDist_{0, conf.maxPriceOffset},
      qtyDist_{conf.minQty, conf.maxQty},
      eventDist_{conf.mix.add, conf.mix.modify, conf.mix.cancel},
      crossDist_{conf.crossProbability},
      tifDist_{conf.tifMix.gtc, conf.tifMix.ioc, conf.tifMix.fok}, nextId_{0} {
  liveOrders_.reserve(prefillCount() + conf.warmupCount + conf.eventCount);
}

Event EventGenerator::emitAddEvent() {
  Side side = sideDist_(rng_) ? Side::Bid : Side::Ask;
  Price price = side == Side::Bid ? config_.midPrice - bidOffsetDist_(rng_)
                                  : config_.midPrice + askOffsetDist_(rng_);
  Quantity qty = qtyDist_(rng_);

  Event event{.eventType = EventType::Add,
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

  if (liveOrder.qty <= 1) {
    return emitCancelEvent();
  }

  std::uniform_int_distribution<Quantity> newQtyDist{1, liveOrder.qty - 1};
  Quantity newQty = newQtyDist(rng_);

  liveOrder.qty = newQty;

  return Event{.eventType = EventType::Modify,
               .id = liveOrder.id,
               .side = Side{},
               .price = Price{},
               .qty = newQty};
}

Event EventGenerator::emitCancelEvent() {
  auto liveIndex = pickLiveIndex();
  auto& liveOrder = liveOrders_[liveIndex];

  Event event = Event{.eventType = EventType::Cancel,
                      .id = liveOrder.id,
                      .side = Side{},
                      .price = Price{},
                      .qty = {}};

  liveOrder = liveOrders_.back();
  liveOrders_.pop_back();

  return event;
}

std::size_t EventGenerator::prefillCount() const {
  if (config_.mix.add >= 0.5) {
    return 0;
  }

  return config_.warmupCount + config_.eventCount;
}

Workload EventGenerator::generate() {
  Workload workload;
  const auto prefill = prefillCount();
  const auto sampledEvents = config_.warmupCount + config_.eventCount;
  workload.reserve(prefill + sampledEvents);

  for (std::size_t i{}; i < prefill; ++i) {
    workload.push_back(emitAddEvent());
  }

  for (std::size_t i{}; i < sampledEvents; ++i) {
    auto eventName = static_cast<EventType>(eventDist_(rng_));

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
