#include "config.h"
#include "order_book/deque_fat_order_book.h"
#include "order_book/deque_iter_order_book.h"
#include "order_book/deque_order_book.h"
#include "order_book/list_iter_order_book.h"
#include "order_book/list_order_book.h"
#include "order_book/list_ptr_order_book.h"
#include "parse_args.h"
#include "runner/run_benchmark.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
  try {
    RunConfig config = parseArgs(argc, argv);

    switch (config.impl) {
    case Impl::Deque:
      runBenchmark<DequeOrderBook>(config);
      break;
    case Impl::List:
      runBenchmark<ListOrderBook>(config);
      break;
    case Impl::DequeIter:
      runBenchmark<DequeIterOrderBook>(config);
      break;
    case Impl::ListIter:
      runBenchmark<ListIterOrderBook>(config);
      break;
    case Impl::ListPtr:
      runBenchmark<ListPtrOrderBook>(config);
      break;
    case Impl::DequeFat:
      runBenchmark<DequeFatOrderBook>(config);
      break;
    }
  } catch (const std::exception& err) {
    std::cerr << "error: " << err.what() << "\n";
    return 1;
  }

  return 0;
}
