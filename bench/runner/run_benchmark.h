#pragma once

#include "order_book/order_book.h"
#include "output/run_metadata.h"
#include "output/write_latencies.h"
#include "types.h"
#include "workload/event_generator.h"
#include "workload_runners.h"

#include <filesystem>
#include <iostream>
#include <span>

template <OrderBookLike Book>
void runBenchmark(const RunConfig& runConf) {
  EventGenerator eventGenerator(runConf.workload);

  Workload events = eventGenerator.generate();

  std::span<const Event> workload{events};

  auto warmupWorkload = workload.first(runConf.workload.warmupCount);
  auto benchWorkload = workload.subspan(runConf.workload.warmupCount,
                                        runConf.workload.eventCount);

  Book book;

  runUntimed(book, warmupWorkload);
  auto records = runTimed(book, benchWorkload);

  RunMetadata meta = captureRunMetadata();
  std::filesystem::create_directories(runConf.outputDir);

  std::string rawFile = "raw_" + meta.runId + ".bin";
  std::string metaFile = "meta_" + meta.runId + ".json";
  std::string rawPath = runConf.outputDir + "/" + rawFile;
  std::string metaPath = runConf.outputDir + "/" + metaFile;

  writeLatencies(rawPath, records);
  writeMetadataJson(metaPath, meta, runConf, rawFile, records.size(),
                    sizeof(LatencyRecord));

  std::cout << "runId: " << meta.runId << "\n"
            << "raw: " << rawPath << "\n"
            << "meta: " << metaPath << "\n"
            << "records: " << records.size() << "\n";
}
