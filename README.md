# order-books

A C++20 project for implementing and benchmarking limit order books against a
common interface.

The project provides:

- A C++20 concept (`OrderBookLike`) that defines the order book interface.
- A reference implementation, `GhostBook`, used by the test suite and the
  workload generator.
- An empty user implementation, `OrderBook`, to fill in.
- A GoogleTest typed-test suite covering resting, matching, market orders,
  time-in-force, modify, and cancel behavior.
- Two benchmark drivers: a raw-latency harness that records per-op latencies,
  and a Google Benchmark harness.
- A Python CLI (`order-book-analyze`) for summarizing and plotting raw benchmark
  output.

## Contents

- [Layout](#layout)
- [Prerequisites](#prerequisites)
- [Build](#build)
- [The `OrderBookLike` concept](#the-orderbooklike-concept)
- [Implementing `OrderBook`](#implementing-orderbook)
- [Tests](#tests)
- [Benchmarks](#benchmarks)
- [Analysis CLI](#analysis-cli)
- [Flag reference](#flag-reference)
- [Adding more implementations](#adding-more-implementations)

## Layout

```
include/order_book/
  order_book.h          OrderBookLike concept
  order_book_impl.h     class OrderBook (declarations)
  types.h               Order, Trade, Side, OrderType, TimeInForce

src/
  order_book_impl.cpp   class OrderBook (stub bodies)

bench/
  config.h              RunConfig, WorkloadConfig, Impl enum
  harness/
    main.cpp            Raw-latency benchmark entry point
    gbench_main.cpp     Google Benchmark entry point
    parse_args.{h,cpp}  CLI flag parsing
  output/               Latency record + JSON metadata writers
  runner/               Templated benchmark loop (runTimed/runUntimed)
  workload/
    event_generator.*   Workload generator
    ghost_book.{h,cpp}  Reference implementation
    event.h             Event types

tests/
  order_book_behavior_test.cpp   Typed-test suite

analysis/
  src/order_book_analysis/       Python CLI: summarize, plot

docs/
  adding_implementation.md       Notes on adding additional impls
```

`src/` contains only the user implementation. `GhostBook` lives under
`bench/workload/`.

## Prerequisites

- C++20 compiler (GCC 12+ or Clang 15+)
- CMake 3.20+
- Python 3.12+ (for the analysis CLI)

GoogleTest and Google Benchmark are fetched via CMake `FetchContent`.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_BENCH=ON
cmake --build build -j
```

`BUILD_TESTS` and `BUILD_BENCH` are independent and both default to `OFF`.

## The `OrderBookLike` concept

Defined in `include/order_book/order_book.h`:

```cpp
template <typename T>
concept OrderBookLike =
    requires(T& book, const T& constBook, Order order, OrderId id, Side side,
             Price price, Quantity qty) {
      { book.addOrder(order) } -> std::same_as<Trades>;
      { book.modifyOrder(id, qty) } -> std::same_as<bool>;
      { book.cancelOrder(id) } -> std::same_as<bool>;

      { constBook.bestBid() } -> std::same_as<std::optional<std::pair<Price, Quantity>>>;
      { constBook.bestAsk() } -> std::same_as<std::optional<std::pair<Price, Quantity>>>;

      { constBook.qtyAt(side, price) } -> std::same_as<Quantity>;
      { constBook.depth(side) } -> std::same_as<std::size_t>;
      { constBook.orderCount() } -> std::same_as<std::size_t>;
    };
```

| Method                                   | Description                                                                     |
| ---------------------------------------- | ------------------------------------------------------------------------------- |
| `addOrder(Order) -> Trades`              | Submit an order. Returns trades produced if it crossed; empty vector otherwise. |
| `modifyOrder(OrderId, Quantity) -> bool` | Change the quantity of a live order. `false` if the id is unknown.              |
| `cancelOrder(OrderId) -> bool`           | Remove a live order. `false` if the id is unknown.                              |
| `bestBid() / bestAsk()`                  | `(price, total qty)` at the top of the side, or `std::nullopt` if empty.        |
| `qtyAt(Side, Price)`                     | Total resting quantity at a price level; `0` if empty.                          |
| `depth(Side)`                            | Number of distinct price levels on a side.                                      |
| `orderCount()`                           | Total number of live orders.                                                    |

Types are in `include/order_book/types.h`.

## Implementing `OrderBook`

Two files:

- `include/order_book/order_book_impl.h` — declarations.
- `src/order_book_impl.cpp` — bodies.

Replace the stubs. Sources under `src/` are picked up by `file(GLOB_RECURSE)`,
so no CMake changes are needed for additional source files.

To run the test suite against your implementation, edit
`tests/order_book_behavior_test.cpp`:

```cpp
using FullImpl = ::testing::Types<OrderBook>;
```

To list multiple types:

```cpp
using FullImpl = ::testing::Types<GhostBook, OrderBook>;
```

## Tests

```sh
ctest --test-dir build --output-on-failure
```

The suite is split into six typed test classes — `RestingTest`, `MatchingTest`,
`MarketOrderTest`, `TifTest`, `ModifyTest`, `CancelTest` — each instantiated for
every type in `FullImpl`. As shipped, `FullImpl` is
`::testing::Types<GhostBook>`.

GoogleTest filters work as usual:

```sh
./build/tests/order_books_tests --gtest_filter='MatchingTest*'
```

## Benchmarks

### Raw latency

`order_books_bench` runs a workload through one implementation, timing every
operation with `steady_clock`. Output:

- `raw_<runId>.bin` — packed
  `LatencyRecord{int64_t latencyNs, uint8_t eventType}` rows.
- `meta_<runId>.json` — workload config, impl, git commit, build flags,
  timestamp, record count.

Example:

```sh
./build/bench/order_books_bench \
  --impl=Ghost \
  --size=1000000 \
  --warmup=100000 \
  --add-ratio=0.6 --modify-ratio=0.2 --cancel-ratio=0.2 \
  --cross-prob=0.1 \
  --gtc-ratio=1.0 --ioc-ratio=0.0 --fok-ratio=0.0 \
  --max-price-offset=50 \
  --seed=67 \
  --output-dir=artifacts/bench
```

`--size` is the number of measured events; `--warmup` is the number of warmup
events run before measurement. The generator also runs an internal prefill pass
before warmup to populate the book.

The realized event mix can drift from the requested ratios under certain
conditions (e.g. modify on a qty-1 order becomes cancel, cross requests with
no opposing liquidity are dropped). See
[`docs/benchmark_drift.md`](docs/benchmark_drift.md) for the full catalogue.

### Google Benchmark

```sh
./build/bench/order_books_gbench
./build/bench/order_books_gbench --benchmark_filter='Ghost'
./build/bench/order_books_gbench --benchmark_repetitions=5
```

Writes a `meta_<runId>.json` to `$BENCH_OUTPUT_DIR` (default `artifacts/bench`).

## Analysis CLI

```sh
cd analysis
python3 -m venv .venv
. .venv/bin/activate
pip install -e .
```

Summarize percentiles:

```sh
order-book-analyze summarize artifacts/bench
order-book-analyze summarize artifacts/bench --impl Ghost --impl OrderBook
order-book-analyze summarize artifacts/bench --out summary.csv
```

Plot:

```sh
order-book-analyze plot artifacts/bench --out plots/
order-book-analyze plot artifacts/bench --out plots/ --xlimit 0,5000
order-book-analyze plot artifacts/bench --out plots/ --impl OrderBook --run-id 20260515_120000_abcd
```

`--impl` and `--run-id` are repeatable; values within a flag are OR'd.

## Flag reference

### `order_books_bench`

| Flag                  | Type                 | Default           | Description                                          |
| --------------------- | -------------------- | ----------------- | ---------------------------------------------------- |
| `--impl=`             | `Ghost \| OrderBook` | _(required)_      | Implementation to run.                               |
| `--size=`             | int                  | `1000000`         | Number of measured events.                           |
| `--warmup=`           | int                  | `100000`          | Number of warmup events.                             |
| `--seed=`             | int                  | `67`              | RNG seed.                                            |
| `--add-ratio=`        | float                | `0.6`             | Probability of an add event.                         |
| `--modify-ratio=`     | float                | `0.2`             | Probability of a modify event.                       |
| `--cancel-ratio=`     | float                | `0.2`             | Probability of a cancel event.                       |
| `--cross-prob=`       | float                | `0.0`             | Probability an add is generated to cross the spread. |
| `--gtc-ratio=`        | float                | `1.0`             | Share of GTC orders.                                 |
| `--ioc-ratio=`        | float                | `0.0`             | Share of IOC orders.                                 |
| `--fok-ratio=`        | float                | `0.0`             | Share of FOK orders.                                 |
| `--max-price-offset=` | int                  | `50`              | Max distance from mid for generated prices.          |
| `--output-dir=`       | path                 | `artifacts/bench` | Where `raw_*.bin` and `meta_*.json` are written.     |

Event-mix and TIF-mix ratios are passed to `std::discrete_distribution` and do
not need to sum to 1.

### `order-book-analyze`

```
order-book-analyze summarize <results_dir> [--impl X]... [--run-id Y]... [--out PATH]
order-book-analyze plot      <results_dir>  --out DIR  [--impl X]... [--run-id Y]... [--xlimit lo,hi] [--force]
```

## Adding more implementations

See [`docs/adding_implementation.md`](docs/adding_implementation.md).
