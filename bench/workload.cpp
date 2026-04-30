#include "workload.h"

#include "order_book/types.h"

#include <cstddef>
#include <optional>
#include <random>
#include <vector>

namespace {

constexpr Price midPrice = 10000;
constexpr Price priceOffset = 50;
constexpr Quantity minQty = 1;
constexpr Quantity maxQty = 100;

constexpr int addWeight = 50;
constexpr int modifyWeight = 20;
constexpr int cancelWeight = 30;

struct OrderInfo {
  OrderId id;
  Quantity qty;
};

} // namespace

void generateWorkload(Events& events, std::size_t size, std::uint64_t seed) {
  events.clear();
  events.reserve(size);

  std::vector<OrderInfo> liveOrders;
  liveOrders.reserve(size);
  OrderId curId = 0;

  std::mt19937_64 rng{seed};

  auto getLiveIdx = [&]() {
    std::uniform_int_distribution<std::size_t> idxDist{0,
                                                       liveOrders.size() - 1};
    return idxDist(rng);
  };

  auto cancelOrder = [&](std::size_t idx) {
    events.push_back({.op = Operation::Cancel,
                      .id = liveOrders[idx].id,
                      .newQty = std::nullopt,
                      .order = std::nullopt});

    liveOrders[idx] = liveOrders.back();
    liveOrders.pop_back();
  };

  std::discrete_distribution<int> opDist{addWeight, modifyWeight, cancelWeight};

  std::bernoulli_distribution sideDist{0.5};
  std::uniform_int_distribution<Price> offsetDist{1, priceOffset};
  std::uniform_int_distribution<Quantity> qtyDist{minQty, maxQty};

  for (std::size_t i{}; i < size; ++i) {
    auto op = static_cast<Operation>(opDist(rng));

    if (op != Operation::Add && liveOrders.empty()) {
      op = Operation::Add;
    }

    switch (op) {
    case Operation::Add: {
      OrderId id = ++curId;
      Side side = sideDist(rng) ? Side::Bid : Side::Ask;
      Price price = side == Side::Bid ? midPrice - offsetDist(rng)
                                      : midPrice + offsetDist(rng);
      Quantity qty = qtyDist(rng);

      Order order = {.id = id,
                     .side = side,
                     .type = OrderType::Limit,
                     .tif = TimeInForce::GTC,
                     .price = price,
                     .qty = qty};

      events.push_back({.op = Operation::Add,
                        .id = std::nullopt,
                        .newQty = std::nullopt,
                        .order = order});

      liveOrders.push_back({.id = id, .qty = qty});

      break;
    }
    case Operation::Modify: {
      auto idx = getLiveIdx();
      OrderInfo& orderInfo = liveOrders[idx];

      // Modify can only decrease quantity while remaining > 0
      // If quantity is 1 there is no valid reduction, so cancel instead
      if (orderInfo.qty == 1) {
        cancelOrder(idx);
        break;
      }

      std::uniform_int_distribution<Quantity> newQtyDist{minQty,
                                                         orderInfo.qty - 1};

      Quantity newQty = newQtyDist(rng);

      events.push_back({.op = Operation::Modify,
                        .id = orderInfo.id,
                        .newQty = newQty,
                        .order = std::nullopt});

      orderInfo.qty = newQty;

      break;
    }
    case Operation::Cancel: {
      auto idx = getLiveIdx();

      cancelOrder(idx);

      break;
    }
    }
  }
}
