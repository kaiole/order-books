#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>

class MapListOrderBook {
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
    Quantity totalQty{0};
    std::list<OrderId> queue;
  };

  struct OrderInfo {
    Order order;
    std::list<OrderId>::iterator orderIt;
  };

  std::map<Price, LevelInfo> bids_;
  std::map<Price, LevelInfo> asks_;
  std::unordered_map<OrderId, OrderInfo> orders_;

  void matchOrder(Order& order, Trades& trades);

  bool canCross(const Order& order, Price restingPrice) const;

  void insertResting(const Order& order);

  bool canFullyFill(const Order& order) const;
};
