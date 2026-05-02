#pragma once

#include "generateWorkload.h"
#include "order_book/order_book.h"
#include "parse_args.h"
#include "runWorkload.h"

#include <span>

template <OrderBookLike Book>
void benchImpl(const Args& args) {
  Workload events =
      generateWorkload(args.workloadSize + args.warmupSize, args.seed);

  std::span<const Event> workload{events};

  auto warmupWorkload = workload.first(args.warmupSize);
  auto benchWorkload = workload.subspan(args.warmupSize);

  Book book;
  // runWorkload(book, warmupWorkload);
  runWorkload(book, benchWorkload);

  // TODO: fork() generation/ warmup phase or find way to isolate it from perf
}
