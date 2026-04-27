#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>

class MapDequeOrderBook {
public:
  Trades addOrder(Order order);

  bool modifyOrder(OrderId id, Quantity newQty);

  bool cancelOrder(OrderId id);

  std::optional<std::pair<Price, Quantity>> bestBid() const;
  std::optional<std::pair<Price, Quantity>> bestAsk() const;

  Quantity qtyAt(Side side, Price price) const;
  std::size_t depth(Side side) const;
  std::size_t orderCount() const;

private:
  struct LevelInfo {
    Quantity totalQty;
    std::deque<OrderId> orders;
  };

  std::map<Price, LevelInfo, std::greater<Price>> bids_;
  std::map<Price, LevelInfo, std::less<Price>> asks_;
  std::unordered_map<OrderId, Order> orders_;

  void matchOrder(Order& order, Trades& trades);

  void insertResting(const Order& order);

  bool canFullyFill(const Order& order);
};
