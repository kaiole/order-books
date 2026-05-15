# Benchmark workload drift

The benchmark CLI flags describe the *intended* workload. The workload
generator in `bench/workload/event_generator.cpp` makes several deliberate
substitutions that cause the realized workload to drift from those flag
values. None of these are bugs — they exist to keep the workload runnable
under conditions that would otherwise produce invalid operations — but they
mean the realized statistics will not exactly match the requested mix.

This document catalogues every drift source, what triggers it, and what to
expect in the resulting `raw_*.bin` and `meta_*.json`.

## Summary

| # | Drift | Trigger | Effect |
| --- | --- | --- | --- |
| 1 | Modify becomes Cancel | Modify picked on a live order with `qty <= 1` | Realized cancel rate > `--cancel-ratio`, modify rate < `--modify-ratio` |
| 2 | Modify/Cancel becomes Add | Modify or Cancel picked while no live orders exist | Realized add rate > `--add-ratio` |
| 3 | Cross silently dropped | `--cross-prob > 0` but opposing side is empty | Realized cross rate < `--cross-prob` |
| 4 | IOC/FOK never go resting | Add rolls IOC or FOK | Live population grows slower than `--add-ratio` implies |
| 5 | Modify is decrease-only | All modifies | Implementations never see a quantity-increase modify |
| 6 | Prefill ignores TIF + cross flags | `prefillCount() > 0` | First N events are forced GTC, non-crossing |

---

## 1. Event-type drift

These two substitutions change the operation that gets emitted, so they
directly affect the realized add/modify/cancel ratios.

### 1a. Modify becomes Cancel when `qty <= 1`

`event_generator.cpp`, `emitModifyEvent`:

```cpp
if (liveOrder.qty <= 1) {
  return emitCancelEvent();
}
```

Modify chooses a new quantity uniformly from `[1, liveOrder.qty - 1]`. If
the current qty is `1`, there is no valid lower value, so the generator
emits a cancel instead.

**Result:** with a heavy modify mix, expect cancel rate to creep above
`--cancel-ratio`. The effect grows with how much modify drives quantities
toward 1.

### 1b. Modify/Cancel becomes Add when book is empty

`event_generator.cpp`, `generate`:

```cpp
if (eventName != EventType::Add && liveOrders_.empty()) {
  eventName = EventType::Add;
}
```

A modify or cancel against an empty live-order pool has nothing to act on,
so the generator substitutes an add. This is most visible on the first few
events of any cancel/modify-heavy workload where prefill is disabled
(`--add-ratio >= 0.5` and `--cross-prob=0.0` skips prefill entirely — see
`prefillCount`).

**Result:** realized add rate > `--add-ratio` near the start of the
workload, or anywhere the live pool transiently empties.

---

## 2. Realized-rate drift

These don't change which operation is emitted; they change what the
operation does. Event-type counts stay close to the configured ratios, but
the workload's behavior deviates from what the other flags imply.

### 2a. Cross silently dropped when opposite side is empty

`event_generator.cpp`, `emitAddEvent`:

```cpp
bool rolledCross = crossDist_(rng_);
// ...
bool shouldCross = !forceRest && rolledCross;
auto opposite = side == Side::Bid ? ghost_.bestAsk() : ghost_.bestBid();

if (shouldCross && opposite) {
  // crossing price path
} else {
  // normal (non-crossing) price path
}
```

If `rolledCross` is true but the opposing side is empty, the add falls
through to the non-crossing pricing path. The cross is silently dropped, not
retried.

**Result:** realized cross rate ≤ `--cross-prob`. This shows up most when
the opposing side gets depleted faster than the add ratio refills it, or
during prefill (which forces non-crossing anyway — see drift #6). The code
comment above `prefillCount` notes this explicitly.

### 2b. Prefill ignores TIF and cross flags

`event_generator.cpp`, `generate`:

```cpp
for (std::size_t i{}; i < prefill; ++i) {
  workload.push_back(emitAddEvent(true));   // forceRest = true
}
```

`emitAddEvent(forceRest=true)` overrides the rolled TIF to `GTC` and forces
`shouldCross = false`:

```cpp
TimeInForce tif = forceRest ? TimeInForce::GTC : rolledTif;
bool shouldCross = !forceRest && rolledCross;
```

Prefill runs whenever `prefillCount() > 0`, which currently means any time
`--add-ratio < 0.5` *or* `--cross-prob > 0`. When prefill runs, its event
count is `warmupCount + eventCount` — i.e. prefill is roughly the same size
as the measured region.

**Result:** the first ~`prefillCount()` events in the workload do not
honor `--gtc-ratio`, `--ioc-ratio`, `--fok-ratio`, or `--cross-prob` at
all. Since `prefill` events run *before* warmup, none of them are timed —
but their side-effect on the book shape (all GTC, all non-crossing) carries
into the measured region.

---

## 3. Population-shape drift

These don't affect event counts or realized rates either; they change the
shape of the live order pool that the workload is acting on.

### 3a. IOC/FOK orders never go resting

`event_generator.cpp`, `emitAddEvent`:

```cpp
if (tif == TimeInForce::IOC || tif == TimeInForce::FOK || qty == 0) {
  return event;
}

liveIndex_.emplace(nextId_, liveOrders_.size());
liveOrders_.push_back(LiveOrder{.id = nextId_, .qty = qty});
```

IOC and FOK orders are emitted as `EventType::Add` (counted in the add
ratio) but never enter the live-order pool, regardless of whether they
matched. Likewise, any add that fully fills on entry has `qty == 0` and
won't go resting.

**Result:** with non-trivial `--ioc-ratio` or `--fok-ratio`, the live pool
grows slower than `--add-ratio` suggests. Mixed with a heavy modify/cancel
ratio, this increases how often the empty-book substitution from drift #1b
fires.

### 3b. Modify is strictly decrease-only

`event_generator.cpp`, `emitModifyEvent`:

```cpp
std::uniform_int_distribution<Quantity> newQtyDist{1, liveOrder.qty - 1};
Quantity newQty = newQtyDist(rng_);
```

`newQty` is sampled from `[1, liveOrder.qty - 1]`, so a modify always
reduces quantity. Implementations are never exercised with a
quantity-increase modify.

**Result:** modify-heavy workloads steadily drive live-order quantities
toward `1`, at which point further modifies on those orders trigger drift
#1a (become cancels). Net effect over a long run: heavier cancel pressure
than the configured ratio.

---

## Diagnosing drift in a run

The metadata file does not currently break out realized vs. requested event
counts, but the raw file does — each `LatencyRecord` carries an `eventType`
byte. A quick way to check realized ratios from Python:

```python
import numpy as np
raw = np.fromfile("raw_<runId>.bin", dtype=[("ns", "<i8"), ("type", "u1")])
unique, counts = np.unique(raw["type"], return_counts=True)
# 0 = Add, 1 = Modify, 2 = Cancel  (see workload/event.h)
print(dict(zip(unique, counts / counts.sum())))
```

Compare against the `add`, `modify`, `cancel` fields under `workload.mix`
in the matching `meta_<runId>.json`.

## Mitigations

If a particular drift source is biasing comparisons between implementations,
the usual mitigations are:

- **Drift #1a (modify → cancel on `qty <= 1`):** lower the modify ratio, or
  raise `--max-qty` / `--min-qty` so quantities take longer to walk down to
  1.
- **Drift #1b (op → add on empty book):** ensure `prefillCount() > 0` so the
  book has depth before the measured region starts — keep `--add-ratio
  < 0.5` or `--cross-prob > 0`.
- **Drift #2a (cross dropped):** raise `--max-price-offset` and the add
  ratio to maintain deeper opposing-side liquidity.
- **Drift #2b (prefill ignores flags):** the prefill phase isn't timed, so
  this doesn't bias measurements directly. If you care about the book shape
  going into the measured region, either keep `prefillCount() == 0` (heavy
  add, no cross) or accept that the first ~`prefillCount()` events shape an
  all-GTC, non-crossing book.
- **Drift #3a (IOC/FOK don't rest):** offset with a higher `--add-ratio`,
  or measure on workloads where `--gtc-ratio = 1.0`.
- **Drift #3b (decrease-only modify):** there's no flag-level mitigation
  today; this is a property of the generator.
