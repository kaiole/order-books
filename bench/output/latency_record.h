#pragma once

#include <cstdint>

struct LatencyRecord {
  std::int64_t latencyNs;
  std::uint8_t eventType;
  std::uint8_t pad[7]{};
};

static_assert(sizeof(LatencyRecord) == 16);

struct RawHeader {
  char magic[8];
  std::uint32_t version;
  std::uint32_t recordSize;
  std::uint64_t recordCount;
};

static_assert(sizeof(RawHeader) == 24);
