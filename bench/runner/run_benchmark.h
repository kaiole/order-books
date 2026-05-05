#pragma once

#include "order_book/order_book.h"
#include "run_workload.h"
#include "types.h"
#include "workload/event_generator.h"

#include <span>

template <OrderBookLike Book>
void runBenchmark(const RunConfig& runConf) {
  EventGenerator eventGenerator(runConf.workload);

  Workload events = eventGenerator.generate();

  std::span<const Event> workload{events};

  auto warmupWorkload = workload.first(runConf.workload.warmupCount);
  auto benchWorkload = workload.subspan(runConf.workload.warmupCount,
                                        runConf.workload.eventCount);

  Book book;

  runWorkload(book, warmupWorkload);
  runWorkload(book, benchWorkload);
}
