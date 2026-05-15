#include "order_book/order_book_impl.h"

Trades OrderBook::addOrder(Order /*order*/) { return {}; }

bool OrderBook::modifyOrder(OrderId /*id*/, Quantity /*newQty*/) {
  return false;
}

bool OrderBook::cancelOrder(OrderId /*id*/) { return false; }

std::optional<std::pair<Price, Quantity>> OrderBook::bestBid() const {
  return std::nullopt;
}

std::optional<std::pair<Price, Quantity>> OrderBook::bestAsk() const {
  return std::nullopt;
}

Quantity OrderBook::qtyAt(Side /*side*/, Price /*price*/) const { return 0; }

std::size_t OrderBook::depth(Side /*side*/) const { return 0; }

std::size_t OrderBook::orderCount() const { return 0; }
