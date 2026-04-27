#pragma once

#include <cstdint>
#include <vector>

using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::int64_t;

enum class Side : uint8_t { Bid, Ask };

enum class OrderType : std::uint8_t { Limit, Market };

enum class TimeInForce {
  GTC, // Good-Till-Cancel
  IOC, // Immediate-Or-Cancel
  FOK, // Fill-Or-Kill
};

struct Order {
  OrderId id;
  Side side;
  OrderType type;
  TimeInForce tif;
  Price price;
  Quantity qty;
};

struct Trade {
  OrderId agressorId;
  OrderId passiveId;
  Price price;
  Quantity qty;
};

using Trades = std::vector<Trade>;
