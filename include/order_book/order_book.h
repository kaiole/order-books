#pragma once

#include "types.h"

#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>

template <typename T>
concept OrderBookLike =
    requires(T& book, const T& constBook, Order order, OrderId id, Side side,
             Price price, Quantity qty) {
      { book.addOrder(order) } -> std::same_as<Trades>;
      { book.modifyOrder(id, qty) } -> std::same_as<bool>;
      { book.cancelOrder(id) } -> std::same_as<bool>;

      {
        constBook.bestBid()
      } -> std::same_as<std::optional<std::pair<Price, Quantity>>>;
      {
        constBook.bestAsk()
      } -> std::same_as<std::optional<std::pair<Price, Quantity>>>;

      { constBook.qtyAt(side, price) } -> std::same_as<Quantity>;
      { constBook.depth(side) } -> std::same_as<std::size_t>;
      { constBook.orderCount() } -> std::same_as<std::size_t>;
    };
