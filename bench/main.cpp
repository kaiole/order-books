#include "order_book/map_deque_order_book.h"
#include "order_book/map_list_order_book.h"
#include "parse_args.h"
#include "run_benchmark.h"
#include "types.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
  try {
    BenchConf benchConf = parseArgs(argc, argv);

    switch (benchConf.impl) {
    case Impl::MapDeque:
      runBenchmark<MapDequeOrderBook>(benchConf);
      break;
    case Impl::MapList:
      runBenchmark<MapListOrderBook>(benchConf);
      break;
    }
  } catch (const std::exception& err) {
    std::cerr << "error: " << err.what() << "\n";
  }

  return 0;
}
