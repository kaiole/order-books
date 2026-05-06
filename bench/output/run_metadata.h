#pragma once

#include "types.h"

#include <string>

struct RunMetadata {
  std::string runId;
  std::string timestampIso8601;
  std::string gitCommit;
  std::string compilerId;
  std::string compilerVersion;
  std::string buildType;
  std::string buildFlags;
  std::string cxxStandard;
  std::string hostname;
  std::string cpuModel;
  std::string cpuGovernor;
};

RunMetadata captureRunMetadata();

std::string implName(Impl impl);

void writeMetadataJson(const std::string& path, const RunMetadata& meta,
                       const RunConfig& run, const std::string& rawFile,
                       std::size_t recordCount, std::size_t recordSizeBytes);
