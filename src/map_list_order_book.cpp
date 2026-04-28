#include "order_book/map_list_order_book.h"

#include "order_book/types.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>

Trades MapListOrderBook::addOrder(Order order) {
  Trades trades;

  if (order.qty <= 0 || orders_.contains(order.id)) {
    return trades;
  }

  switch (order.tif) {
  case TimeInForce::GTC:
    matchOrder(order, trades);
    if (order.qty > 0 && order.type == OrderType::Limit) {
      insertResting(order);
    }
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

bool MapListOrderBook::modifyOrder(OrderId id, Quantity newQty) {
  auto orderIt = orders_.find(id);

  if (orderIt == orders_.end()) {
    return false;
  }

  auto& orderInfo = orderIt->second;
  auto& order = orderInfo.order;
  if (newQty >= order.qty || newQty <= 0) {
    return false;
  }

  auto& levels = (order.side == Side::Bid) ? bids_ : asks_;
  auto levelIt = levels.find(order.price);

  if (levelIt == levels.end()) {
    return false;
  }

  auto& level = levelIt->second;

  level.totalQty += newQty - order.qty;
  order.qty = newQty;

  return true;
}

bool MapListOrderBook::cancelOrder(OrderId id) {
  auto orderIt = orders_.find(id);

  if (orderIt == orders_.end()) {
    return false;
  }

  auto& orderInfo = orderIt->second;
  const auto& order = orderInfo.order;

  auto& levels = (order.side == Side::Bid) ? bids_ : asks_;

  auto levelIt = levels.find(order.price);
  if (levelIt == levels.end()) {
    return false;
  }

  auto& level = levelIt->second;

  level.queue.erase(orderInfo.orderIt);
  level.totalQty -= order.qty;

  if (level.queue.empty()) {
    levels.erase(levelIt);
  }

  orders_.erase(orderIt);

  return true;
}

std::optional<std::pair<Price, Quantity>> MapListOrderBook::bestBid() const {
  if (bids_.empty()) {
    return std::nullopt;
  }

  const auto& [price, level] = *bids_.rbegin();
  return std::pair<Price, Quantity>{price, level.totalQty};
}

std::optional<std::pair<Price, Quantity>> MapListOrderBook::bestAsk() const {
  if (asks_.empty()) {
    return std::nullopt;
  }

  const auto& [price, level] = *asks_.begin();
  return std::pair<Price, Quantity>{price, level.totalQty};
}

Quantity MapListOrderBook::qtyAt(Side side, Price price) const {
  const auto& levels = (side == Side::Bid) ? bids_ : asks_;
  const auto levelIt = levels.find(price);

  if (levelIt == levels.end()) {
    return 0;
  }

  return levelIt->second.totalQty;
}

std::size_t MapListOrderBook::depth(Side side) const {
  return (side == Side::Bid) ? bids_.size() : asks_.size();
}

std::size_t MapListOrderBook::orderCount() const { return orders_.size(); }

void MapListOrderBook::matchOrder(Order& order, Trades& trades) {
  auto& levels = (order.side == Side::Bid) ? asks_ : bids_;

  while (order.qty > 0 && !levels.empty()) {
    auto levelIt =
        (order.side == Side::Bid) ? levels.begin() : std::prev(levels.end());
    auto restingPrice = levelIt->first;

    if (!canCross(order, restingPrice)) {
      break;
    }

    auto& queue = levelIt->second.queue;

    while (order.qty > 0 && !queue.empty()) {
      auto restingIt = orders_.find(queue.front());
      auto& orderInfo = restingIt->second;
      auto& restingOrder = orderInfo.order;

      Quantity filledQty = std::min(order.qty, restingOrder.qty);

      order.qty -= filledQty;
      restingOrder.qty -= filledQty;
      levelIt->second.totalQty -= filledQty;

      trades.push_back({.aggressorId = order.id,
                        .passiveId = restingOrder.id,
                        .price = restingOrder.price,
                        .qty = filledQty});

      if (restingOrder.qty == 0) {
        queue.pop_front();
        orders_.erase(restingIt);
      }
    }

    if (queue.empty()) {
      levels.erase(levelIt);
    }
  }
}

bool MapListOrderBook::canCross(const Order& order, Price restingPrice) const {
  if (order.type == OrderType::Market) {
    return true;
  }

  if (order.side == Side::Bid) {
    return order.price >= restingPrice;
  }

  return order.price <= restingPrice;
}

void MapListOrderBook::insertResting(const Order& order) {
  auto& levels = (order.side == Side::Bid) ? bids_ : asks_;
  auto& level = levels[order.price];

  level.totalQty += order.qty;
  level.queue.push_back(order.id);
  auto orderIt = std::prev(level.queue.end());

  orders_[order.id] = {.order = order, .orderIt = orderIt};
}

bool MapListOrderBook::canFullyFill(const Order& order) const {
  auto remainingAfterFill = [this, &order]<typename T>(T first, T last) {
    Quantity curQty{order.qty};

    for (auto it = first;
         it != last && curQty != 0 && canCross(order, it->first); ++it) {
      curQty -= std::min(curQty, it->second.totalQty);
    }

    return curQty;
  };

  if (order.side == Side::Bid) {
    return remainingAfterFill(asks_.begin(), asks_.end()) == 0;
  }

  return remainingAfterFill(bids_.rbegin(), bids_.rend()) == 0;
}
