#include "order_book/list_ptr_order_book.h"

#include "order_book/types.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>

Trades ListPtrOrderBook::addOrder(Order order) {
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

bool ListPtrOrderBook::modifyOrder(OrderId id, Quantity newQty) {
  auto it = orders_.find(id);

  if (it == orders_.end()) {
    return false;
  }

  auto& record = it->second;
  auto& order = *(*record.orderIt);
  if (newQty >= order.qty || newQty <= 0) {
    return false;
  }

  auto& level = record.levelIt->second;

  level.totalQty += newQty - order.qty;
  order.qty = newQty;

  return true;
}

bool ListPtrOrderBook::cancelOrder(OrderId id) {
  auto it = orders_.find(id);

  if (it == orders_.end()) {
    return false;
  }

  auto& record = it->second;
  auto& order = *(*record.orderIt);

  auto& levels = order.side == Side::Bid ? bids_ : asks_;
  auto levelIt = record.levelIt;
  auto& level = levelIt->second;

  level.totalQty -= order.qty;
  level.queue.erase(record.orderIt);

  if (level.queue.empty()) {
    levels.erase(levelIt);
  }

  orders_.erase(it);

  return true;
}

std::optional<std::pair<Price, Quantity>> ListPtrOrderBook::bestBid() const {
  if (bids_.empty()) {
    return std::nullopt;
  }

  const auto& [price, level] = *bids_.rbegin();
  return std::pair<Price, Quantity>{price, level.totalQty};
}

std::optional<std::pair<Price, Quantity>> ListPtrOrderBook::bestAsk() const {
  if (asks_.empty()) {
    return std::nullopt;
  }

  const auto& [price, level] = *asks_.begin();
  return std::pair<Price, Quantity>{price, level.totalQty};
}

Quantity ListPtrOrderBook::qtyAt(Side side, Price price) const {
  const auto& levels = side == Side::Bid ? bids_ : asks_;
  const auto levelIt = levels.find(price);

  if (levelIt == levels.end()) {
    return 0;
  }

  return levelIt->second.totalQty;
}

std::size_t ListPtrOrderBook::depth(Side side) const {
  return side == Side::Bid ? bids_.size() : asks_.size();
}

std::size_t ListPtrOrderBook::orderCount() const { return orders_.size(); }

void ListPtrOrderBook::matchOrder(Order& order, Trades& trades) {
  auto& levels = order.side == Side::Bid ? asks_ : bids_;

  while (order.qty > 0 && !levels.empty()) {
    auto levelIt =
        order.side == Side::Bid ? levels.begin() : std::prev(levels.end());
    const Price restingPrice = levelIt->first;

    if (!canCross(order, restingPrice)) {
      break;
    }

    auto& level = levelIt->second;
    auto& queue = level.queue;

    while (order.qty > 0 && !queue.empty()) {
      auto& restingOrder = *queue.front();

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

bool ListPtrOrderBook::canCross(const Order& order, Price restingPrice) const {
  if (order.type == OrderType::Market) {
    return true;
  }

  if (order.side == Side::Bid) {
    return order.price >= restingPrice;
  }

  return order.price <= restingPrice;
}

void ListPtrOrderBook::insertResting(const Order& order) {
  auto& levels = order.side == Side::Bid ? bids_ : asks_;
  auto levelIt = levels.try_emplace(order.price).first;
  auto& level = levelIt->second;

  level.queue.push_back(std::make_unique<Order>(order));

  auto orderIt = std::prev(level.queue.end());
  orders_.try_emplace(order.id, OrderRecord{orderIt, levelIt});

  level.totalQty += order.qty;
}

bool ListPtrOrderBook::canFullyFill(const Order& order) const {
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
