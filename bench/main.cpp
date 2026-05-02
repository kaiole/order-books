#include "benchImpl.h"
#include "order_book/map_deque_order_book.h"
#include "order_book/map_list_order_book.h"
#include "parse_args.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
  try {
    Args args = parseArgs(argc, argv);

    if (args.impl == Impl::MapDeque) {
      benchImpl<MapDequeOrderBook>(args);
    } else {
      benchImpl<MapListOrderBook>(args);
    }
  } catch (const std::exception& err) {
    std::cerr << "error: " << err.what() << "\n";
  }

  return 0;
}
