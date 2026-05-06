#pragma once

#include "order_book/order_book.h"
#include "order_book/types.h"
#include "output/latency_record.h"
#include "workload/event.h"

#include <benchmark/benchmark.h>
#include <chrono>
#include <span>
#include <vector>

template <OrderBookLike Book>
[[nodiscard]] std::vector<LatencyRecord>
runTimed(Book& book, std::span<const Event> workload) {
  auto latencyNs = []<typename T>(T t0, T t1) -> std::int64_t {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
        .count();
  };

  std::vector<LatencyRecord> times;
  times.reserve(workload.size());

  for (std::size_t i{}; i < workload.size(); ++i) {
    const Event& event = workload[i];

    switch (event.eventType) {
    case EventType::Add: {
      auto t0 = std::chrono::steady_clock::now();
      auto trades =
          book.addOrder(Order{event.id, event.side, OrderType::Limit,
                              TimeInForce::GTC, event.price, event.qty});
      benchmark::DoNotOptimize(trades);
      auto t1 = std::chrono::steady_clock::now();

      LatencyRecord record = {.latencyNs = latencyNs(t0, t1),
                              .eventType =
                                  static_cast<std::uint8_t>(event.eventType)};
      times.push_back(record);
      break;
    }

    case EventType::Modify: {
      auto t0 = std::chrono::steady_clock::now();
      bool status = book.modifyOrder(event.id, event.qty);
      benchmark::DoNotOptimize(status);
      auto t1 = std::chrono::steady_clock::now();

      LatencyRecord record = {.latencyNs = latencyNs(t0, t1),
                              .eventType =
                                  static_cast<std::uint8_t>(event.eventType)};
      times.push_back(record);
      break;
    }

    case EventType::Cancel: {
      auto t0 = std::chrono::steady_clock::now();
      bool status = book.cancelOrder(event.id);
      benchmark::DoNotOptimize(status);
      auto t1 = std::chrono::steady_clock::now();

      LatencyRecord record = {.latencyNs = latencyNs(t0, t1),
                              .eventType =
                                  static_cast<std::uint8_t>(event.eventType)};
      times.push_back(record);
      break;
    }
    }
  }

  return times;
}

template <OrderBookLike Book>
void runUntimed(Book& book, std::span<const Event> workload) {
  for (std::size_t i{}; i < workload.size(); ++i) {
    const Event& event = workload[i];

    switch (event.eventType) {
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
