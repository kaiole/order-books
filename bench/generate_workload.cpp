#include "generate_workload.h"

#include "order_book/types.h"

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace {

constexpr Price midPrice = 10000;
constexpr Price maxPriceOffset = 50;
constexpr Quantity minQty = 1;
constexpr Quantity maxQty = 100;

constexpr int addOrderWeight = 50;
constexpr int modifyOrderWeight = 25;
constexpr int cancelOrderWeight = 25;

struct OrderInfo {
  OrderId id;
  Quantity qty;
};

} // namespace

Workload generateWorkload(std::size_t size, std::uint64_t seed) {
  Workload workload;
  workload.reserve(size);

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
    workload.push_back(Event{
        .op = Operation::Cancel,
        .id = liveOrders[idx].id,
        .qty = Quantity{},
        .side = Side{},
        .price = Price{},
    });

    liveOrders[idx] = liveOrders.back();
    liveOrders.pop_back();
  };

  std::discrete_distribution<int> opDist{addOrderWeight, modifyOrderWeight,
                                         cancelOrderWeight};

  std::bernoulli_distribution sideDist{0.5};
  std::uniform_int_distribution<Price> offsetDist{1, maxPriceOffset};
  std::uniform_int_distribution<Quantity> qtyDist{minQty, maxQty};

  for (std::size_t i{}; i < size; ++i) {
    auto op = static_cast<Operation>(opDist(rng));

    if (op != Operation::Add && liveOrders.empty()) {
      op = Operation::Add;
    }

    switch (op) {
    case Operation::Add: {
      const OrderId id = ++curId;
      const Side side = sideDist(rng) ? Side::Bid : Side::Ask;
      const Price price = side == Side::Bid ? midPrice - offsetDist(rng)
                                            : midPrice + offsetDist(rng);
      const Quantity qty = qtyDist(rng);

      workload.push_back(Event{
          .op = Operation::Add,
          .id = id,
          .qty = qty,
          .side = side,
          .price = price,
      });

      liveOrders.push_back(OrderInfo{
          .id = id,
          .qty = qty,
      });

      break;
    }

    case Operation::Modify: {
      const auto idx = getLiveIdx();
      OrderInfo& orderInfo = liveOrders[idx];

      if (orderInfo.qty == 1) {
        cancelOrder(idx);
        break;
      }

      std::uniform_int_distribution<Quantity> newQtyDist{minQty,
                                                         orderInfo.qty - 1};
      const Quantity newQty = newQtyDist(rng);

      workload.push_back(Event{
          .op = Operation::Modify,
          .id = orderInfo.id,
          .qty = newQty,
          .side = Side{},
          .price = Price{},
      });

      orderInfo.qty = newQty;

      break;
    }

    case Operation::Cancel: {
      const auto idx = getLiveIdx();
      cancelOrder(idx);
      break;
    }
    }
  }

  return workload;
}
