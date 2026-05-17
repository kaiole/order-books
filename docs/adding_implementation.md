# Adding an order book implementation

This document covers wiring a third order book implementation into the project
alongside `GhostBook` and `OrderBook`. The same steps apply if you want to keep
several variants side-by-side and benchmark them against each other.

Throughout this guide the new implementation is called `MyBook`, in files named
`my_book.{h,cpp}`. Substitute your own names.

## Overview of touch-points

| File                                        | Edit                                                   |
| ------------------------------------------- | ------------------------------------------------------ |
| `include/order_book/my_book.h`              | Declare `class MyBook`.                                |
| `src/my_book.cpp`                           | Define `MyBook`'s methods.                             |
| `bench/config.h`                            | Add an `Impl` enum value.                              |
| `bench/harness/parse_args.cpp`              | Parse the new `--impl=` value, update usage string.    |
| `bench/harness/main.cpp`                    | Dispatch the new enum value to `runBenchmark<MyBook>`. |
| `bench/harness/gbench_main.cpp`             | Register a `BENCHMARK(...)` for `MyBook`.              |
| `bench/output/run_metadata.cpp`             | Add a case in `implName`.                              |
| `tests/order_book_behavior_test.cpp`        | Concept assertion + add to `FullImpl`.                 |
| `analysis/src/order_book_analysis/plots.py` | (Optional) add an entry to `IMPL_COLORS`.              |

CMake auto-globs `src/*.cpp` and `tests/*.cpp`, so no `CMakeLists.txt` edits are
required.

The walkthrough below covers each step in order.

## 1. Header — `include/order_book/my_book.h`

Declare the public surface required by the `OrderBookLike` concept (defined in
`include/order_book/order_book.h`):

```cpp
#pragma once

#include "order_book/types.h"

#include <cstddef>
#include <optional>
#include <utility>

class MyBook {
public:
  Trades addOrder(Order order);
  bool modifyOrder(OrderId id, Quantity newQty);
  bool cancelOrder(OrderId id);

  std::optional<std::pair<Price, Quantity>> bestBid() const;
  std::optional<std::pair<Price, Quantity>> bestAsk() const;

  Quantity qtyAt(Side side, Price price) const;
  std::size_t depth(Side side) const;
  std::size_t orderCount() const;

  // ... your private members and helpers
};
```

The concept is checked at compile time via the `OrderBookLike` constraint on the
benchmark and test templates, so signature mismatches surface as compilation
errors rather than silent miscompiles.

## 2. Source — `src/my_book.cpp`

```cpp
#include "order_book/my_book.h"

// definitions ...
```

Picked up automatically by
`file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "src/*.cpp")` in the top-level
`CMakeLists.txt`.

## 3. Bench config — `bench/config.h`

Add the new value to the `Impl` enum:

```cpp
enum class Impl : std::uint8_t { Ghost, OrderBook, MyBook };
```

The compiler will warn (`-Wswitch`) on any switch over `Impl` that doesn't
handle the new value — the next three steps fix those.

## 4. CLI parser — `bench/harness/parse_args.cpp`

Add a branch in `parseImpl`. Accept both the PascalCase form and an
all-lowercase alias:

```cpp
if (value == "MyBook" || value == "mybook") {
  return Impl::MyBook;
}
```

Update the usage string in the `missing required args` error message:

```cpp
"--impl=<Ghost|OrderBook|MyBook> "
```

## 5. CLI dispatcher — `bench/harness/main.cpp`

Include the header and add a case to the switch on `config.impl`:

```cpp
#include "order_book/my_book.h"

// ...

case Impl::MyBook:
  runBenchmark<MyBook>(config);
  break;
```

`runBenchmark<Book>` is the templated entry point in
`bench/runner/run_benchmark.h`. It enforces `OrderBookLike<Book>` and handles
prefill, warmup, timed measurement, and writing `raw_*.bin` / `meta_*.json`.

## 6. Google Benchmark harness — `bench/harness/gbench_main.cpp`

Include the header and register a benchmark:

```cpp
#include "order_book/my_book.h"

// ...

BENCHMARK(benchDefault<MyBook>)
    ->Name("MyBook/default")
    ->Args({1'000'000, 100'000})
    ->Unit(benchmark::kMillisecond);
```

The `->Name(...)` label is what shows up in the gbench output and in any
metadata downstream tooling reads.

## 7. Metadata — `bench/output/run_metadata.cpp`

Add a case in `implName`'s switch:

```cpp
case Impl::MyBook:
  return "MyBook";
```

The returned string is what gets written into `meta_*.json` under the `"impl"`
key, and is what the analysis CLI groups by when you pass `--impl MyBook`.

## 8. Tests — `tests/order_book_behavior_test.cpp`

Three edits:

1. Include the header:

   ```cpp
   #include "order_book/my_book.h"
   ```

2. Add a static concept check (compile-time sanity):

   ```cpp
   static_assert(OrderBookLike<MyBook>);
   ```

3. Add the type to `FullImpl` so the typed suites instantiate against it:

   ```cpp
   using FullImpl = ::testing::Types<GhostBook, MyBook>;
   ```

The six typed test classes (`RestingTest`, `MatchingTest`, `MarketOrderTest`,
`TifTest`, `ModifyTest`, `CancelTest`) all share `FullImpl`, so a single edit
runs every behavior test against the new impl.

## 9. (Optional) Plot colors — `analysis/src/order_book_analysis/plots.py`

If you plan to plot results, add a color for the new impl in `IMPL_COLORS`:

```python
IMPL_COLORS: dict[str, str] = {
    "Ghost":     "#1f77b4",
    "OrderBook": "#ff7f0e",
    "MyBook":    "#2ca02c",
}
```

Without an entry the plotter falls back to a default color cycle, so this is
only useful if you want consistent colors across runs.

## 10. Verify

Build everything, run tests, run a smoke benchmark:

```sh
cmake --build build -j

ctest --test-dir build --output-on-failure

./build/bench/order_books_bench --impl=MyBook \
  --size=10000 --warmup=1000 \
  --add-ratio=0.6 --modify-ratio=0.2 --cancel-ratio=0.2 \
  --output-dir=artifacts/bench

./build/bench/order_books_gbench --benchmark_filter='MyBook'
```

A successful run produces a `raw_<runId>.bin` and `meta_<runId>.json` pair in
`artifacts/bench`. From there:

```sh
order-book-analyze summarize artifacts/bench --impl MyBook
order-book-analyze plot      artifacts/bench --out plots/ --impl MyBook
```

## Common pitfalls

- **Concept failure at template instantiation.** If `runBenchmark<MyBook>` or
  `static_assert(OrderBookLike<MyBook>)` fails to compile, the error message
  will reference `OrderBookLike` and the specific clause that didn't match.
  Check the exact return type and `const`-qualification on each method signature
  against `include/order_book/order_book.h` — these must match exactly (e.g.
  `Quantity`, not `int64_t`; `std::size_t`, not `unsigned long` on every
  platform).

- **Unhandled enum value.** A `-Wswitch` warning means a switch over `Impl`
  somewhere missed the new value. Check `parse_args.cpp`, `main.cpp`, and
  `run_metadata.cpp`.

- **Metadata label mismatch.** If `implName(Impl::MyBook)` doesn't return the
  same string the parser accepts (e.g. `"MyBook"` vs `"My_Book"`), the analysis
  CLI's `--impl` filter won't match your metadata. Keep them identical.

- **Header not in `include/order_book/`.** The tests and harness includes use
  the `order_book/` prefix (`#include "order_book/my_book.h"`); a header placed
  elsewhere in the tree will not be found by those translation units.
