#include "config.h"
#include "workload/ghost_book.h"
#include "order_book/order_book_impl.h"
#include "parse_args.h"
#include "runner/run_benchmark.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
  try {
    RunConfig config = parseArgs(argc, argv);

    switch (config.impl) {
    case Impl::Ghost:
      runBenchmark<GhostBook>(config);
      break;
    case Impl::OrderBook:
      runBenchmark<OrderBook>(config);
      break;
    }
  } catch (const std::exception& err) {
    std::cerr << "error: " << err.what() << "\n";
    return 1;
  }

  return 0;
}
