#pragma once

#include "order_book/types.h"

#include <cstdint>
#include <vector>

enum class EventType : std::uint8_t { Add = 0, Modify = 1, Cancel = 2 };

struct Event {
  EventType eventType;
  OrderId id;
  Side side;
  Price price;
  Quantity qty;
};

using Workload = std::vector<Event>;
