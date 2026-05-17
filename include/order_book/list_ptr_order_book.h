#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

class ListPtrOrderBook {
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
  using OrderPtr = std::unique_ptr<Order>;

  struct LevelInfo {
    Quantity totalQty{0};
    std::list<OrderPtr> queue;
  };

  using LevelMap = std::map<Price, LevelInfo>;
  using QueueIterator = std::list<OrderPtr>::iterator;

  struct OrderRecord {
    QueueIterator orderIt;
    LevelMap::iterator levelIt;
  };

  LevelMap bids_;
  LevelMap asks_;
  std::unordered_map<OrderId, OrderRecord> orders_;

  void matchOrder(Order& order, Trades& trades);

  bool canCross(const Order& order, Price restingPrice) const;

  void insertResting(const Order& order);

  bool canFullyFill(const Order& order) const;
};
