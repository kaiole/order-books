#include "order_book/map_deque_order_book.h"

#include "order_book/types.h"

#include <optional>
#include <utility>

// Trades MapOrderBook::addOrder(Order order);
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

Quantity MapOrderBook::qtyAt(Side side, Price price) const {
  if (side == Side::Bid) {
    if (auto levelIt = bids_.find(price); levelIt != bids_.end()) {
      return levelIt->second.totalQty;
    }
  }

  if (side == Side::Ask) {
    if (auto levelIt = asks_.find(price); levelIt != bids_.end()) {
      return levelIt->second.totalQty;
    }
  }

  return 0;
}

std::size_t MapOrderBook::depth(Side side) const {
  if (side == Side::Bid) {
    return bids_.size();
  }

  return asks_.size();
}

std::size_t MapOrderBook::orderCount() const { return orders_.size(); }
