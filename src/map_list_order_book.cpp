#include "order_book/map_list_order_book.h"

#include "order_book/types.h"

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
  auto it = orders_.find(id);

  if (it == orders_.end()) {
    return false;
  }

  auto& orderIt = it->second;
  if (newQty >= orderIt->qty || newQty <= 0) {
    return false;
  }

  auto& levels = orderIt->side == Side::Bid ? bids_ : asks_;
  auto levelIt = levels.find(orderIt->price);

  if (levelIt == levels.end()) {
    return false;
  }

  auto& level = levelIt->second;

  level.totalQty += newQty - orderIt->qty;
  orderIt->qty = newQty;

  return true;
}

bool MapListOrderBook::cancelOrder(OrderId id) {
  auto it = orders_.find(id);

  if (it == orders_.end()) {
    return false;
  }

  auto& orderIt = it->second;

  auto& levels = orderIt->side == Side::Bid ? bids_ : asks_;
  auto levelIt = levels.find(orderIt->price);
  if (levelIt == levels.end()) {
    return false;
  }

  auto& level = levelIt->second;

  level.totalQty -= orderIt->qty;
  level.queue.erase(orderIt);

  if (level.queue.empty()) {
    levels.erase(levelIt);
  }

  orders_.erase(it);

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
  const auto& levels = side == Side::Bid ? bids_ : asks_;
  const auto levelIt = levels.find(price);

  if (levelIt == levels.end()) {
    return 0;
  }

  return levelIt->second.totalQty;
}

std::size_t MapListOrderBook::depth(Side side) const {
  return side == Side::Bid ? bids_.size() : asks_.size();
}

std::size_t MapListOrderBook::orderCount() const { return orders_.size(); }

void MapListOrderBook::matchOrder(Order& order, Trades& trades) {
  auto& levels = order.side == Side::Bid ? asks_ : bids_;

  while (order.qty > 0 && !levels.empty()) {
    auto levelIt =
        order.side == Side::Bid ? levels.begin() : std::prev(levels.end());
    const Price restingPrice = levelIt->first;

    if (!canCross(order, restingPrice)) {
      break;
    }

    auto& level = levelIt->second;
    auto& queue = levelIt->second.queue;

    while (order.qty > 0 && !queue.empty()) {
      auto& restingOrder = queue.front();

      Quantity filledQty = std::min(order.qty, restingOrder.qty);

      order.qty -= filledQty;
      restingOrder.qty -= filledQty;
      level.totalQty -= filledQty;

      trades.push_back(Trade{.aggressorId = order.id,
                             .passiveId = restingOrder.id,
                             .price = restingOrder.price,
                             .qty = filledQty});

      if (restingOrder.qty == 0) {
        orders_.erase(restingOrder.id);
        queue.pop_front();
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
  auto& levels = order.side == Side::Bid ? bids_ : asks_;
  auto levelIt = levels.try_emplace(order.price).first;
  auto& level = levelIt->second;

  level.queue.push_back(order);

  auto orderIt = std::prev(level.queue.end());
  orders_.try_emplace(order.id, orderIt);

  level.totalQty += order.qty;
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
