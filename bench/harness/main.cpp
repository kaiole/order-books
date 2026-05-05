#include "order_book/map_deque_order_book.h"
#include "order_book/map_list_order_book.h"
#include "parse_args.h"
#include "runner/run_benchmark.h"
#include "types.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
  try {
    BenchConfig config = parseArgs(argc, argv);

    switch (config.impl) {
    case Impl::MapDeque:
      runBenchmark<MapDequeOrderBook>(config);
      break;
    case Impl::MapList:
      runBenchmark<MapListOrderBook>(config);
      break;
    }
  } catch (const std::exception& err) {
    std::cerr << "error: " << err.what() << "\n";
    return 1;
  }

  return 0;
}
