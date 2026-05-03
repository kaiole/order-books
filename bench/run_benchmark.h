#pragma once

#include "generate_workload.h"
#include "order_book/order_book.h"
#include "run_perf.h"
#include "run_workload.h"
#include "types.h"

#include <span>

template <OrderBookLike Book>
void runBenchmark(const BenchConf& benchConf) {
  Workload events = generateWorkload(
      benchConf.workloadSize + benchConf.warmupSize, benchConf.seed);

  std::span<const Event> workload{events};

  auto warmupWorkload = workload.first(benchConf.warmupSize);
  auto benchWorkload = workload.subspan(benchConf.warmupSize);

  Book book;

  runWorkload(book, warmupWorkload);
  runPerf(benchConf.impl);
  runWorkload(book, benchWorkload);
}
