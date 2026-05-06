#include "output/write_latencies.h"

#include "types.h"

#include <fstream>
#include <ios>
#include <span>
#include <stdexcept>
#include <string_view>

void writeLatencies(std::string_view path,
                    std::span<const LatencyRecord> records) {
  std::ofstream out(std::string(path), std::ios::binary);

  if (!out) {
    throw std::runtime_error("failed to open latency output file");
  }

  const RawHeader header{
      .magic = {'O', 'R', 'D', 'B', 'O', 'O', 'K', '\0'},
      .version = 1,
      .recordSize = sizeof(LatencyRecord),
      .recordCount = records.size(),
  };

  out.write(reinterpret_cast<const char*>(&header), sizeof(header));

  out.write(
      reinterpret_cast<const char*>(records.data()),
      static_cast<std::streamsize>(records.size() * sizeof(LatencyRecord)));

  if (!out) {
    throw std::runtime_error("failed to write latency records");
  }
}
