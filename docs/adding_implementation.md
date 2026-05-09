# Adding a new order book implementation

Touch-points required end-to-end. CMake auto-globs sources, so no `CMakeLists.txt` edit is needed.

## 1. Header — `include/order_book/<name>_order_book.h`

Public surface must satisfy the `OrderBookLike` concept (`include/order_book/order_book.h`):

- `Trades addOrder(Order)`
- `bool modifyOrder(OrderId, Quantity)`
- `bool cancelOrder(OrderId)`
- `std::optional<std::pair<Price, Quantity>> bestBid() const`
- `std::optional<std::pair<Price, Quantity>> bestAsk() const`
- `Quantity qtyAt(Side, Price) const`
- `std::size_t depth(Side) const`
- `std::size_t orderCount() const`

## 2. Source — `src/<name>_order_book.cpp`

Picked up automatically by `file(GLOB_RECURSE LIB_SOURCES ...)` in `CMakeLists.txt`.

## 3. Tests — `tests/order_book_behavior_test.cpp`

- Add `#include "order_book/<name>_order_book.h"`.
- Add `static_assert(OrderBookLike<NewBook>);`.
- Add the type to `FullImpl` — the typed suites then run on it automatically.

## 4. Bench config — `bench/config.h`

Add a new value to `enum class Impl`.

## 5. CLI parser — `bench/harness/parse_args.cpp`

- Add a branch in `parseImpl` accepting both PascalCase and lowercase forms.
- Update the usage string in the `missing required args` error.

## 6. CLI dispatcher — `bench/harness/main.cpp`

- Add the header include.
- Add a `case Impl::New: runBenchmark<NewBook>(config); break;` to the switch.

## 7. Metadata — `bench/output/run_metadata.cpp`

Add a case in `implName`'s switch. `-Wswitch` will warn if missed.

`bench/runner/run_perf.cpp` has a similar switch in `getPerfOutputPath`. The `-Wswitch` warning is acceptable until you actually want to perf-record the new impl — at that point add `./perf/samples/<name>.data`.

## 8. gbench harness — `bench/harness/gbench_main.cpp`

- Add the header include.
- Register: `BENCHMARK(benchDefault<NewBook>)->Name("New/default")->Args({1'000'000, 100'000})->Unit(benchmark::kMillisecond);`

## 9. Verify

```sh
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Summary

| File | Purpose |
| --- | --- |
| `include/order_book/<name>_order_book.h` | new class declaration |
| `src/<name>_order_book.cpp` | implementation |
| `tests/order_book_behavior_test.cpp` | concept assertion + add to `FullImpl` |
| `bench/config.h` | extend `Impl` enum |
| `bench/harness/parse_args.cpp` | flag parsing + usage string |
| `bench/harness/main.cpp` | CLI dispatch |
| `bench/output/run_metadata.cpp` | impl name string |
| `bench/harness/gbench_main.cpp` | gbench registration |
