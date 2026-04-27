#include "order_book/map_deque_order_book.h"

#include "order_book/types.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>

Trades MapDequeOrderBook::addOrder(Order order) {
  Trades trades;

  switch (order.tif) {
  case TimeInForce::GTC:
    matchOrder(order, trades);
    insertResting(order);
    break;

  case TimeInForce::IOC:
    matchOrder(order, trades);
    break;

  case TimeInForce::FOK:
    if (canFullyFill(order)) {
      matchOrder(order, trades);
    }
    break;
  }

  return trades;
}

// bool MapOrderBook::modifyOrder(OrderId id, Quantity newQty);
// bool MapOrderBook::cancelOrder(OrderId id);

std::optional<std::pair<Price, Quantity>> MapDequeOrderBook::bestBid() const {
  if (bids_.empty()) {
    return std::nullopt;
  }

  const auto& [price, level] = *bids_.rbegin();
  return std::pair<Price, Quantity>{price, level.totalQty};
}

std::optional<std::pair<Price, Quantity>> MapDequeOrderBook::bestAsk() const {
  if (asks_.empty()) {
    return std::nullopt;
  }

  const auto& [price, level] = *asks_.begin();
  return std::pair<Price, Quantity>{price, level.totalQty};
}

Quantity MapDequeOrderBook::qtyAt(Side side, Price price) const {
  const auto& levels = (side == Side::Bid) ? bids_ : asks_;
  if (const auto& levelIt = levels.find(price); levelIt != levels.end()) {
    return levelIt->second.totalQty;
  }

  return 0;
}

std::size_t MapDequeOrderBook::depth(Side side) const {
  return (side == Side::Bid) ? bids_.size() : asks_.size();
}

std::size_t MapDequeOrderBook::orderCount() const { return orders_.size(); }

void MapDequeOrderBook::matchOrder(Order& order, Trades& trades) {
  auto& levels = (order.side == Side::Bid) ? asks_ : bids_;

  while (order.qty > 0 && !levels.empty()) {
    auto levelIt =
        (order.side == Side::Bid) ? levels.begin() : std::prev(levels.end());
    auto restingPrice = levelIt->first;

    if (canCross(order, restingPrice)) {
      break;
    }

    auto& dq = levelIt->second.queue;

    while (order.qty > 0 && !dq.empty()) {
      auto restingIt = orders_.find(dq.front());
      auto& restingOrder = restingIt->second;

      Quantity filledQty = std::min(order.qty, restingOrder.qty);

      order.qty -= filledQty;
      restingOrder.qty -= filledQty;
      levelIt->second.totalQty -= filledQty;

      trades.push_back({.agressorId = order.id,
                        .passiveId = restingOrder.id,
                        .price = restingOrder.price,
                        .qty = filledQty});

      if (restingOrder.qty == 0) {
        dq.pop_front();
        orders_.erase(restingIt);
      }
    }

    if (dq.empty()) {
      levels.erase(levelIt);
    }
  }
}

bool MapDequeOrderBook::canCross(const Order& order, Price restingPrice) const {
  if (order.type == OrderType::Market) {
    return true;
  }

  if (order.side == Side::Bid) {
    return order.price >= restingPrice;
  }

  return order.price <= restingPrice;
}

bool MapDequeOrderBook::canFullyFill(const Order& order) const {
  auto fillAgainst = [this, &order]<typename T>(T first, T last) {
    Quantity curQty{order.qty};

    for (auto it = first;
         it != last && curQty != 0 && canCross(order, it->first); ++it) {
      curQty -= std::min(curQty, it->second.totalQty);
    }

    return curQty;
  };

  if (order.side == Side::Bid) {
    return fillAgainst(asks_.begin(), asks_.end()) == 0;
  }

  return fillAgainst(bids_.rbegin(), bids_.rend()) == 0;
}
