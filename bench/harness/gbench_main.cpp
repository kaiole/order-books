#include "order_book/map_deque_order_book.h"
#include "order_book/map_list_order_book.h"
#include "order_book/order_book.h"
#include "output/run_metadata.h"
#include "runner/workload_runners.h"
#include "types.h"
#include "workload/event_generator.h"

#include <benchmark/benchmark.h>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>

namespace {

WorkloadConfig defaultWorkload(std::size_t eventCount,
                               std::size_t warmupCount) {
  WorkloadConfig config;
  config.eventCount = eventCount;
  config.warmupCount = warmupCount;
  config.seed = 67;
  config.mix = EventMix{0.6, 0.2, 0.2};

  return config;
}

template <OrderBookLike Book>
void runOnce(benchmark::State& state, const WorkloadConfig& workloadConf) {
  for (auto _ : state) {
    state.PauseTiming();
    EventGenerator generator(workloadConf);
    Workload events = generator.generate();

    std::span<const Event> all{events};
    auto warmup = all.first(workloadConf.warmupCount);
    auto measured =
        all.subspan(workloadConf.warmupCount, workloadConf.eventCount);

    Book book;
    runUntimed(book, warmup);

    state.ResumeTiming();

    runUntimed(book, measured);

    state.PauseTiming();
    benchmark::DoNotOptimize(book);
    state.ResumeTiming();
  }

  state.SetItemsProcessed(state.iterations() *
                          static_cast<int64_t>(workloadConf.eventCount));
}

template <OrderBookLike Book>
void benchDefault(benchmark::State& state) {
  auto eventCount = static_cast<std::size_t>(state.range(0));
  auto warmupCount = static_cast<std::size_t>(state.range(1));
  runOnce<Book>(state, defaultWorkload(eventCount, warmupCount));
}

void emitMetadata() {
  const char* outDir = std::getenv("BENCH_OUTPUT_DIR");
  std::string dir = outDir ? outDir : "results";
  std::filesystem::create_directories(dir);

  RunMetadata meta = captureRunMetadata();
  RunConfig run;
  run.scenarioName = "default";
  run.outputDir = dir;
  run.workload = defaultWorkload(1'000'000, 100'000);

  std::string path = dir + "/meta_" + meta.runId + ".json";
  writeMetadataJson(path, meta, run, "", 0, 0);
}

} // namespace

BENCHMARK(benchDefault<MapDequeOrderBook>)
    ->Name("MapDeque/default")
    ->Args({1'000'000, 100'000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK(benchDefault<MapListOrderBook>)
    ->Name("MapList/default")
    ->Args({1'000'000, 100'000})
    ->Unit(benchmark::kMillisecond);

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
    return 1;
  }
  emitMetadata();
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
