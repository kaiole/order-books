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
  liveIndex_.reserve(prefillCount() + conf.warmupCount + conf.eventCount);
}

Event EventGenerator::emitAddEvent(bool forceRest) {
  Side side = sideDist_(rng_) ? Side::Bid : Side::Ask;
  TimeInForce rolledTif = static_cast<TimeInForce>(tifDist_(rng_));
  bool rolledCross = crossDist_(rng_);
  TimeInForce tif = forceRest ? TimeInForce::GTC : rolledTif;
  bool shouldCross = !forceRest && rolledCross;
  auto opposite = side == Side::Bid ? ghost_.bestAsk() : ghost_.bestBid();

  Price price;
  if (shouldCross && opposite) {
    Price oppositePrice = opposite->first;
    Price offset = askOffsetDist_(rng_);
    price = side == Side::Bid ? oppositePrice + offset : oppositePrice - offset;
  } else {
    price = side == Side::Bid ? config_.midPrice - bidOffsetDist_(rng_)
                              : config_.midPrice + askOffsetDist_(rng_);
  }

  Quantity qty = qtyDist_(rng_);

  Event event{.eventType = EventType::Add,
              .id = ++nextId_,
              .side = side,
              .tif = tif,
              .price = price,
              .qty = qty};

  auto trades = ghost_.addOrder(Order{.id = event.id,
                                      .side = event.side,
                                      .type = OrderType::Limit,
                                      .tif = event.tif,
                                      .price = event.price,
                                      .qty = event.qty});

  for (auto& trade : trades) {
    applyFill(trade.passiveId, trade.qty);
    qty -= trade.qty;
  }

  if (tif == TimeInForce::IOC || tif == TimeInForce::FOK || qty == 0) {
    return event;
  }

  liveIndex_.emplace(nextId_, liveOrders_.size());
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

  ghost_.modifyOrder(liveOrder.id, newQty);
  liveOrder.qty = newQty;

  return Event{.eventType = EventType::Modify,
               .id = liveOrder.id,
               .side = Side{},
               .tif = TimeInForce{},
               .price = Price{},
               .qty = newQty};
}

Event EventGenerator::emitCancelEvent() {
  auto liveIndex = pickLiveIndex();
  OrderId cancelledId = liveOrders_[liveIndex].id;

  Event event = Event{.eventType = EventType::Cancel,
                      .id = cancelledId,
                      .side = Side{},
                      .tif = TimeInForce{},
                      .price = Price{},
                      .qty = {}};

  ghost_.cancelOrder(cancelledId);
  removeLiveOrder(liveIndex);

  return event;
}

/*******************************************************************************
 * Prefill when the workload may consume or require resting liquidity:
 *  - cancel/modify-heavy mixes need liquidity
 *  - crossing orders can deplete depth
 *    - realized cross rates may still fall below target as depth depletes
 ******************************************************************************/
std::size_t EventGenerator::prefillCount() const {
  if (config_.mix.add >= 0.5 && config_.crossProbability == 0.0) {
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
    workload.push_back(emitAddEvent(true));
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

void EventGenerator::removeLiveOrder(std::size_t index) {
  OrderId removedId = liveOrders_[index].id;

  if (index + 1 != liveOrders_.size()) {
    liveOrders_[index] = liveOrders_.back();
    liveIndex_[liveOrders_[index].id] = index;
  }
  liveOrders_.pop_back();
  liveIndex_.erase(removedId);
}

void EventGenerator::applyFill(OrderId passiveId, Quantity filled) {
  auto index = liveIndex_.at(passiveId);
  auto& liveOrder = liveOrders_[index];
  liveOrder.qty -= filled;

  if (liveOrder.qty == 0) {
    removeLiveOrder(index);
  }
}

std::size_t EventGenerator::pickLiveIndex() {
  if (liveOrders_.empty()) {
    throw std::runtime_error("no live orders");
  }

  auto liveOrderCount = liveOrders_.size();

  std::uniform_int_distribution<std::size_t> indexDist{0, liveOrderCount - 1};

  return indexDist(rng_);
}
