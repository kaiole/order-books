#include "order_book/map_deque_order_book.h"
#include "order_book/map_list_order_book.h"
#include "parse_args.h"
#include "run_benchmark.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
  try {
    BenchConf benchConf = parseArgs(argc, argv);

    if (benchConf.impl == Impl::MapDeque) {
      runBenchmark<MapDequeOrderBook>(benchConf);
    } else {
      runBenchmark<MapListOrderBook>(benchConf);
    }
  } catch (const std::exception& err) {
    std::cerr << "error: " << err.what() << "\n";
  }

  return 0;
}
