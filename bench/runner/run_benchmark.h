#pragma once

#include "order_book/order_book.h"
#include "run_workload.h"
#include "types.h"
#include "workload/event_generator.h"

#include <span>

template <OrderBookLike Book>
void runBenchmark(const BenchConfig& benchConf) {
  EventGenerator eventGenerator(benchConf);

  Workload events = eventGenerator.generate();

  std::span<const Event> workload{events};

  auto warmupWorkload = workload.first(benchConf.warmupCount);
  auto benchWorkload =
      workload.subspan(benchConf.warmupCount, benchConf.eventCount);

  Book book;

  runWorkload(book, warmupWorkload);
  runWorkload(book, benchWorkload);
}
