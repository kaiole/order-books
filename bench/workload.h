#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

enum class Operation : std::int8_t { Add = 0, Modify = 1, Cancel = 2 };

struct Event {
  Operation op;
  std::optional<OrderId> id;
  std::optional<Quantity> newQty;
  std::optional<Order> order;
};

using Events = std::vector<Event>;

void generateWorkload(Events& events, std::size_t size, std::uint64_t seed);
