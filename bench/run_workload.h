#pragma once

#include "order_book/order_book.h"
#include "order_book/types.h"
#include "types.h"

#include <span>

template <OrderBookLike Book>
void runWorkload(Book& book, std::span<const Event> workload) {
  for (std::size_t i{}; i < workload.size(); ++i) {
    const Event& event = workload[i];

    switch (event.op) {
    case Operation::Add: {
      book.addOrder(Order{event.id, event.side, OrderType::Limit,
                          TimeInForce::GTC, event.price, event.qty});
      break;
    }

    case Operation::Modify: {
      book.modifyOrder(event.id, event.qty);
      break;
    }

    case Operation::Cancel: {
      book.cancelOrder(event.id);
      break;
    }
    }
  }
}
