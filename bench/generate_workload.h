#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

enum class Operation : std::int8_t { Add = 0, Modify = 1, Cancel = 2 };

struct Event {
  Operation op;
  OrderId id;
  Quantity qty;
  Side side;
  Price price;
};

using Workload = std::vector<Event>;

Workload generateWorkload(std::size_t size, std::uint64_t seed);
