#include "config.h"
#include "order_book/map_deque_fat_order_book.h"
#include "order_book/map_deque_iter_order_book.h"
#include "order_book/map_deque_order_book.h"
#include "order_book/map_list_iter_order_book.h"
#include "order_book/map_list_order_book.h"
#include "parse_args.h"
#include "runner/run_benchmark.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
  try {
    RunConfig config = parseArgs(argc, argv);

    switch (config.impl) {
    case Impl::MapDeque:
      runBenchmark<MapDequeOrderBook>(config);
      break;
    case Impl::MapList:
      runBenchmark<MapListOrderBook>(config);
      break;
    case Impl::MapDequeIter:
      runBenchmark<MapDequeIterOrderBook>(config);
      break;
    case Impl::MapListIter:
      runBenchmark<MapListIterOrderBook>(config);
      break;
    case Impl::MapDequeFat:
      runBenchmark<MapDequeFatOrderBook>(config);
      break;
    }
  } catch (const std::exception& err) {
    std::cerr << "error: " << err.what() << "\n";
    return 1;
  }

  return 0;
}
