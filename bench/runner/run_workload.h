#pragma once

#include "order_book/order_book.h"
#include "order_book/types.h"
#include "types.h"

#include <benchmark/benchmark.h>
#include <span>

template <OrderBookLike Book>
void runWorkload(Book& book, std::span<const Event> workload) {
  for (std::size_t i{}; i < workload.size(); ++i) {
    const Event& event = workload[i];

    switch (event.eventName) {
    case EventType::Add: {
      auto trades =
          book.addOrder(Order{event.id, event.side, OrderType::Limit,
                              TimeInForce::GTC, event.price, event.qty});
      benchmark::DoNotOptimize(trades);
      break;
    }

    case EventType::Modify: {
      bool status = book.modifyOrder(event.id, event.qty);
      benchmark::DoNotOptimize(status);
      break;
    }

    case EventType::Cancel: {
      bool status = book.cancelOrder(event.id);
      benchmark::DoNotOptimize(status);
      break;
    }
    }
  }
}
