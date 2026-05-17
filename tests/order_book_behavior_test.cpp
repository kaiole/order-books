#include "order_book/deque_fat_order_book.h"
#include "order_book/deque_iter_order_book.h"
#include "order_book/deque_order_book.h"
#include "order_book/list_iter_order_book.h"
#include "order_book/list_order_book.h"
#include "order_book/list_ptr_order_book.h"
#include "order_book/order_book.h"
#include "order_book/types.h"

#include <gtest/gtest.h>
#include <optional>

namespace {

Order makeOrder(OrderId id, Side side, Price price, Quantity qty,
                OrderType type = OrderType::Limit,
                TimeInForce tif = TimeInForce::GTC) {
  return Order{.id = id,
               .side = side,
               .type = type,
               .tif = tif,
               .price = price,
               .qty = qty};
}

static_assert(OrderBookLike<DequeOrderBook>);
static_assert(OrderBookLike<ListOrderBook>);
static_assert(OrderBookLike<DequeIterOrderBook>);
static_assert(OrderBookLike<ListIterOrderBook>);
static_assert(OrderBookLike<ListPtrOrderBook>);
static_assert(OrderBookLike<DequeFatOrderBook>);

template <typename T>
class OrderBookTestBase : public ::testing::Test {
protected:
  T book;
};

template <typename T>
class RestingTest : public OrderBookTestBase<T> {};

template <typename T>
class MatchingTest : public OrderBookTestBase<T> {};

template <typename T>
class MarketOrderTest : public OrderBookTestBase<T> {};

template <typename T>
class TifTest : public OrderBookTestBase<T> {};

template <typename T>
class ModifyTest : public OrderBookTestBase<T> {};

template <typename T>
class CancelTest : public OrderBookTestBase<T> {};

// Run suites on specified implementations
using RestingImpl = ::testing::Types<ListOrderBook>;
using MatchingImpl = ::testing::Types<ListOrderBook>;
using MarketOrderImpl = ::testing::Types<ListOrderBook>;
using TifImpl = ::testing::Types<ListOrderBook>;
using ModifyImpl = ::testing::Types<ListOrderBook>;
using CancelImpl = ::testing::Types<ListOrderBook>;

// TYPED_TEST_SUITE(RestingTest, RestingImpl);
// TYPED_TEST_SUITE(MatchingTest, MatchingImpl);
// TYPED_TEST_SUITE(MarketOrderTest, MarketOrderImpl);
// TYPED_TEST_SUITE(TifTest, TifImpl);
// TYPED_TEST_SUITE(ModifyTest, ModifyImpl);
// TYPED_TEST_SUITE(CancelTest, CancelImpl);

// Run all suites on all implementations
using FullImpl =
    ::testing::Types<DequeOrderBook, ListOrderBook, DequeIterOrderBook,
                     ListIterOrderBook, ListPtrOrderBook, DequeFatOrderBook>;

TYPED_TEST_SUITE(RestingTest, FullImpl);
TYPED_TEST_SUITE(MatchingTest, FullImpl);
TYPED_TEST_SUITE(MarketOrderTest, FullImpl);
TYPED_TEST_SUITE(TifTest, FullImpl);
TYPED_TEST_SUITE(ModifyTest, FullImpl);
TYPED_TEST_SUITE(CancelTest, FullImpl);

/*******************************************************************************
 * Resting Tests
 ******************************************************************************/

TYPED_TEST(RestingTest, EmptyBookHasNoBestPrices) {
  EXPECT_EQ(this->book.bestBid(), std::nullopt);
  EXPECT_EQ(this->book.bestAsk(), std::nullopt);
  EXPECT_EQ(this->book.depth(Side::Bid), 0u);
  EXPECT_EQ(this->book.depth(Side::Ask), 0u);
  EXPECT_EQ(this->book.orderCount(), 0u);
}

TYPED_TEST(RestingTest, RestingLimitInsertsWithoutTrades) {
  auto trades = this->book.addOrder(makeOrder(1, Side::Bid, 100, 10));

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(this->book.bestBid(), (std::pair<Price, Quantity>{100, 10}));
  EXPECT_EQ(this->book.bestAsk(), std::nullopt);
  EXPECT_EQ(this->book.depth(Side::Bid), 1u);
  EXPECT_EQ(this->book.orderCount(), 1u);
}

TYPED_TEST(RestingTest, BestBidIsHighestPrice) {
  this->book.addOrder(makeOrder(1, Side::Bid, 100, 5));
  this->book.addOrder(makeOrder(2, Side::Bid, 102, 3));
  this->book.addOrder(makeOrder(3, Side::Bid, 101, 7));

  EXPECT_EQ(this->book.bestBid(), (std::pair<Price, Quantity>{102, 3}));
  EXPECT_EQ(this->book.depth(Side::Bid), 3u);
}

TYPED_TEST(RestingTest, BestAskIsLowestPrice) {
  this->book.addOrder(makeOrder(1, Side::Ask, 105, 5));
  this->book.addOrder(makeOrder(2, Side::Ask, 103, 3));
  this->book.addOrder(makeOrder(3, Side::Ask, 104, 7));

  EXPECT_EQ(this->book.bestAsk(), (std::pair<Price, Quantity>{103, 3}));
  EXPECT_EQ(this->book.depth(Side::Ask), 3u);
}

TYPED_TEST(RestingTest, MultipleOrdersAtSamePriceAggregateQty) {
  this->book.addOrder(makeOrder(1, Side::Bid, 100, 5));
  this->book.addOrder(makeOrder(2, Side::Bid, 100, 7));
  this->book.addOrder(makeOrder(3, Side::Bid, 100, 3));

  EXPECT_EQ(this->book.qtyAt(Side::Bid, 100), 15);
  EXPECT_EQ(this->book.depth(Side::Bid), 1u);
  EXPECT_EQ(this->book.orderCount(), 3u);
}

TYPED_TEST(RestingTest, QtyAtReturnsZeroForUnknownPrice) {
  this->book.addOrder(makeOrder(1, Side::Bid, 100, 5));

  EXPECT_EQ(this->book.qtyAt(Side::Bid, 99), 0);
  EXPECT_EQ(this->book.qtyAt(Side::Ask, 100), 0);
}

TYPED_TEST(RestingTest, RejectsNonPositiveQuantity) {
  EXPECT_TRUE(this->book.addOrder(makeOrder(1, Side::Bid, 100, 0)).empty());
  EXPECT_TRUE(this->book.addOrder(makeOrder(2, Side::Bid, 100, -5)).empty());

  EXPECT_EQ(this->book.orderCount(), 0u);
  EXPECT_EQ(this->book.depth(Side::Bid), 0u);
}

TYPED_TEST(RestingTest, RejectsDuplicateLiveOrderId) {
  EXPECT_TRUE(this->book.addOrder(makeOrder(1, Side::Bid, 100, 5)).empty());
  EXPECT_TRUE(this->book.addOrder(makeOrder(1, Side::Bid, 101, 7)).empty());

  EXPECT_EQ(this->book.orderCount(), 1u);
  EXPECT_EQ(this->book.bestBid(), (std::pair<Price, Quantity>{100, 5}));
  EXPECT_EQ(this->book.qtyAt(Side::Bid, 101), 0);
}

/*******************************************************************************
 * Matching Tests
 ******************************************************************************/

TYPED_TEST(MatchingTest, CrossingLimitProducesTradeAtRestingPrice) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 10));
  auto trades = this->book.addOrder(makeOrder(2, Side::Bid, 101, 4));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].aggressorId, 2u);
  EXPECT_EQ(trades[0].passiveId, 1u);
  EXPECT_EQ(trades[0].price, 100);
  EXPECT_EQ(trades[0].qty, 4);

  EXPECT_EQ(this->book.bestAsk(), (std::pair<Price, Quantity>{100, 6}));
  EXPECT_EQ(this->book.bestBid(), std::nullopt);
}

TYPED_TEST(MatchingTest, MatchingFollowsFifoAtSameLevel) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 4));
  this->book.addOrder(makeOrder(2, Side::Ask, 100, 5));
  this->book.addOrder(makeOrder(3, Side::Ask, 100, 6));

  auto trades = this->book.addOrder(makeOrder(10, Side::Bid, 100, 7));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].passiveId, 1u);
  EXPECT_EQ(trades[0].qty, 4);
  EXPECT_EQ(trades[1].passiveId, 2u);
  EXPECT_EQ(trades[1].qty, 3);

  EXPECT_EQ(this->book.qtyAt(Side::Ask, 100), 8);
  EXPECT_EQ(this->book.orderCount(), 2u);
}

TYPED_TEST(MatchingTest, MatchingSweepsMultipleLevels) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 3));
  this->book.addOrder(makeOrder(2, Side::Ask, 101, 4));
  this->book.addOrder(makeOrder(3, Side::Ask, 102, 5));

  auto trades = this->book.addOrder(makeOrder(10, Side::Bid, 102, 10));

  ASSERT_EQ(trades.size(), 3u);
  EXPECT_EQ(trades[0].price, 100);
  EXPECT_EQ(trades[1].price, 101);
  EXPECT_EQ(trades[2].price, 102);
  EXPECT_EQ(trades[2].qty, 3);

  EXPECT_EQ(this->book.depth(Side::Ask), 1u);

  EXPECT_EQ(this->book.qtyAt(Side::Ask, 100), 0);
  EXPECT_EQ(this->book.qtyAt(Side::Ask, 101), 0);
  EXPECT_EQ(this->book.qtyAt(Side::Ask, 102), 2);

  EXPECT_EQ(this->book.bestAsk(), (std::pair<Price, Quantity>{102, 2}));
}

TYPED_TEST(MatchingTest, GtcLeavesUnmatchedRemainderResting) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 3));
  auto trades = this->book.addOrder(makeOrder(2, Side::Bid, 100, 10));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].qty, 3);

  EXPECT_EQ(this->book.bestBid(), (std::pair<Price, Quantity>{100, 7}));
  EXPECT_EQ(this->book.bestAsk(), std::nullopt);
}

TYPED_TEST(MatchingTest, NonCrossingLimitDoesNotMatch) {
  this->book.addOrder(makeOrder(1, Side::Ask, 105, 5));
  auto trades = this->book.addOrder(makeOrder(2, Side::Bid, 104, 5));

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(this->book.bestBid(), (std::pair<Price, Quantity>{104, 5}));
  EXPECT_EQ(this->book.bestAsk(), (std::pair<Price, Quantity>{105, 5}));
}

TYPED_TEST(MatchingTest, FullyFilledRestingOrderIsRemoved) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 3));
  this->book.addOrder(makeOrder(2, Side::Bid, 100, 3));

  EXPECT_EQ(this->book.orderCount(), 0u);
  EXPECT_EQ(this->book.bestAsk(), std::nullopt);
}

/*******************************************************************************
 * Market Order Tests
 ******************************************************************************/

TYPED_TEST(MarketOrderTest, MarketOrderCrossesAnyPrice) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 3));
  this->book.addOrder(makeOrder(2, Side::Ask, 200, 4));

  auto trades = this->book.addOrder(
      makeOrder(10, Side::Bid, 0, 5, OrderType::Market, TimeInForce::IOC));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].price, 100);
  EXPECT_EQ(trades[1].price, 200);
  EXPECT_EQ(trades[1].qty, 2);
}

TYPED_TEST(MarketOrderTest, MarketOrderDoesNotRest) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 3));

  auto trades = this->book.addOrder(
      makeOrder(10, Side::Bid, 0, 10, OrderType::Market, TimeInForce::GTC));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].qty, 3);
  EXPECT_EQ(this->book.bestBid(), std::nullopt);
  EXPECT_EQ(this->book.orderCount(), 0u);
}

/*******************************************************************************
 * Time In Force Tests
 ******************************************************************************/

TYPED_TEST(TifTest, IocFillsAvailableThenDiscardsRemainder) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 3));
  auto trades = this->book.addOrder(
      makeOrder(2, Side::Bid, 100, 10, OrderType::Limit, TimeInForce::IOC));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].qty, 3);

  EXPECT_EQ(this->book.bestBid(), std::nullopt);
  EXPECT_EQ(this->book.bestAsk(), std::nullopt);
  EXPECT_EQ(this->book.orderCount(), 0u);
}

TYPED_TEST(TifTest, FokExecutesWhenFullyFillable) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 4));
  this->book.addOrder(makeOrder(2, Side::Ask, 101, 6));

  auto trades = this->book.addOrder(
      makeOrder(10, Side::Bid, 101, 10, OrderType::Limit, TimeInForce::FOK));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].qty, 4);
  EXPECT_EQ(trades[1].qty, 6);
  EXPECT_EQ(this->book.bestAsk(), std::nullopt);
}

TYPED_TEST(TifTest, FokRejectedWhenInsufficientLiquidity) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 4));

  auto trades = this->book.addOrder(
      makeOrder(10, Side::Bid, 100, 10, OrderType::Limit, TimeInForce::FOK));

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(this->book.bestAsk(), (std::pair<Price, Quantity>{100, 4}));
  EXPECT_EQ(this->book.orderCount(), 1u);
}

TYPED_TEST(TifTest, FokRejectedWhenPriceDoesNotCrossEnoughLevels) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 4));
  this->book.addOrder(makeOrder(2, Side::Ask, 105, 10));

  auto trades = this->book.addOrder(
      makeOrder(10, Side::Bid, 100, 10, OrderType::Limit, TimeInForce::FOK));

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(this->book.qtyAt(Side::Ask, 100), 4);
  EXPECT_EQ(this->book.qtyAt(Side::Ask, 105), 10);
}

/*******************************************************************************
 * Modify Tests
 ******************************************************************************/

TYPED_TEST(ModifyTest, ModifyDownReducesQtyAndPreservesPosition) {
  this->book.addOrder(makeOrder(1, Side::Ask, 100, 10));
  this->book.addOrder(makeOrder(2, Side::Ask, 100, 5));

  EXPECT_EQ(this->book.qtyAt(Side::Ask, 100), 15);
  EXPECT_TRUE(this->book.modifyOrder(1, 4));
  EXPECT_EQ(this->book.qtyAt(Side::Ask, 100), 9);

  auto trades = this->book.addOrder(makeOrder(10, Side::Bid, 100, 4));
  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].passiveId, 1u);
  EXPECT_EQ(trades[0].qty, 4);
}

TYPED_TEST(ModifyTest, ModifyUpRejected) {
  this->book.addOrder(makeOrder(1, Side::Bid, 100, 5));

  EXPECT_FALSE(this->book.modifyOrder(1, 10));
  EXPECT_EQ(this->book.qtyAt(Side::Bid, 100), 5);
}

TYPED_TEST(ModifyTest, ModifyToZeroRejected) {
  this->book.addOrder(makeOrder(1, Side::Bid, 100, 5));

  EXPECT_FALSE(this->book.modifyOrder(1, 0));
  EXPECT_EQ(this->book.qtyAt(Side::Bid, 100), 5);
}

TYPED_TEST(ModifyTest, ModifyUnknownOrderReturnsFalse) {
  EXPECT_FALSE(this->book.modifyOrder(42, 1));
}

/*******************************************************************************
 * Cancel Tests
 ******************************************************************************/

TYPED_TEST(CancelTest, CancelRemovesRestingOrder) {
  this->book.addOrder(makeOrder(1, Side::Bid, 100, 5));
  this->book.addOrder(makeOrder(2, Side::Bid, 100, 7));

  EXPECT_TRUE(this->book.cancelOrder(1));

  EXPECT_EQ(this->book.qtyAt(Side::Bid, 100), 7);
  EXPECT_EQ(this->book.orderCount(), 1u);
}

TYPED_TEST(CancelTest, CancelRemovesEmptyLevel) {
  this->book.addOrder(makeOrder(1, Side::Bid, 100, 5));

  EXPECT_TRUE(this->book.cancelOrder(1));

  EXPECT_EQ(this->book.depth(Side::Bid), 0u);
  EXPECT_EQ(this->book.bestBid(), std::nullopt);
}

TYPED_TEST(CancelTest, CancelUnknownOrderReturnsFalse) {
  EXPECT_FALSE(this->book.cancelOrder(42));
}

TYPED_TEST(CancelTest, CancelTwiceReturnsFalseSecondTime) {
  this->book.addOrder(makeOrder(1, Side::Bid, 100, 5));

  EXPECT_TRUE(this->book.cancelOrder(1));
  EXPECT_FALSE(this->book.cancelOrder(1));
}

} // namespace
