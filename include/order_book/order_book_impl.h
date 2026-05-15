#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <optional>
#include <utility>

class OrderBook {
public:
  Trades addOrder(Order order);

  bool modifyOrder(OrderId id, Quantity newQty);

  bool cancelOrder(OrderId id);

  std::optional<std::pair<Price, Quantity>> bestBid() const;
  std::optional<std::pair<Price, Quantity>> bestAsk() const;

  Quantity qtyAt(Side side, Price price) const;
  std::size_t depth(Side side) const;
  std::size_t orderCount() const;
};
