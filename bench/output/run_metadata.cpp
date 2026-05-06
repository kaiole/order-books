#include "run_metadata.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace {

std::string readFirstLine(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    return {};
  }
  std::string line;
  std::getline(file, line);
  return line;
}

std::string readCpuModel() {
  std::ifstream cpuinfo("/proc/cpuinfo");
  if (!cpuinfo) {
    return {};
  }
  std::string line;
  while (std::getline(cpuinfo, line)) {
    if (line.starts_with("model name")) {
      auto pos = line.find(':');
      if (pos != std::string::npos && pos + 2 < line.size()) {
        return line.substr(pos + 2);
      }
    }
  }
  return {};
}

std::string readCpuGovernor() {
  return readFirstLine("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
}

std::string readHostname() {
  char buf[256] = {};
  if (gethostname(buf, sizeof(buf) - 1) != 0) {
    return {};
  }
  return std::string(buf);
}

std::string nowIso8601() {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto t = system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buf);
}

std::string compactTimestampForId() {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto t = system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y%m%dT%H%M%SZ", &tm);
  return std::string(buf);
}

std::string detectCompilerId() {
#if defined(__clang__)
  return "clang";
#elif defined(__GNUC__)
  return "gcc";
#else
  return "unknown";
#endif
}

std::string detectCompilerVersion() {
#if defined(__clang__)
  return std::to_string(__clang_major__) + "." +
         std::to_string(__clang_minor__) + "." +
         std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
  return std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." +
         std::to_string(__GNUC_PATCHLEVEL__);
#else
  return "unknown";
#endif
}

std::string jsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 2);
  for (char c : s) {
    switch (c) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
        out += buf;
      } else {
        out += c;
      }
    }
  }
  return out;
}

void writeKv(std::ostream& os, const std::string& key, const std::string& val,
             bool last) {
  os << "    \"" << key << "\": \"" << jsonEscape(val) << "\"";
  if (!last) {
    os << ",";
  }
  os << "\n";
}

template <typename T>
void writeKvNum(std::ostream& os, const std::string& key, T val, bool last) {
  os << "    \"" << key << "\": " << val;
  if (!last) {
    os << ",";
  }
  os << "\n";
}

} // namespace

std::string implName(Impl impl) {
  switch (impl) {
  case Impl::MapDeque:
    return "MapDeque";

  case Impl::MapList:
    return "MapList";
  }

  return "Unknown";
}

RunMetadata captureRunMetadata() {
  RunMetadata m;
  m.timestampIso8601 = nowIso8601();
  m.runId = compactTimestampForId();

#ifdef BENCH_GIT_COMMIT
  m.gitCommit = BENCH_GIT_COMMIT;
#else
  m.gitCommit = "unknown";
#endif

  m.compilerId = detectCompilerId();
  m.compilerVersion = detectCompilerVersion();

#ifdef BENCH_BUILD_TYPE
  m.buildType = BENCH_BUILD_TYPE;
#else
  m.buildType = "unknown";
#endif

#ifdef BENCH_BUILD_FLAGS
  m.buildFlags = BENCH_BUILD_FLAGS;
#else
  m.buildFlags = "";
#endif

  m.cxxStandard = std::to_string(__cplusplus);

  m.hostname = readHostname();
  m.cpuModel = readCpuModel();
  m.cpuGovernor = readCpuGovernor();

  return m;
}

void writeMetadataJson(const std::string& path, const RunMetadata& meta,
                       const RunConfig& run, const std::string& rawFile,
                       std::size_t recordCount, std::size_t recordSizeBytes) {
  std::ofstream out(path);
  if (!out) {
    return;
  }

  out << "{\n";

  out << "  \"run\": {\n";
  writeKv(out, "runId", meta.runId, false);
  writeKv(out, "timestamp", meta.timestampIso8601, false);
  writeKv(out, "scenario", run.scenarioName, false);
  writeKv(out, "impl", implName(run.impl), true);
  out << "  },\n";

  out << "  \"workload\": {\n";
  writeKvNum(out, "eventCount", run.workload.eventCount, false);
  writeKvNum(out, "warmupCount", run.workload.warmupCount, false);
  writeKvNum(out, "seed", run.workload.seed, false);
  writeKvNum(out, "addRatio", run.workload.mix.add, false);
  writeKvNum(out, "modifyRatio", run.workload.mix.modify, false);
  writeKvNum(out, "cancelRatio", run.workload.mix.cancel, false);
  writeKvNum(out, "midPrice", run.workload.midPrice, false);
  writeKvNum(out, "maxPriceOffset", run.workload.maxPriceOffset, false);
  writeKvNum(out, "minQty", run.workload.minQty, false);
  writeKvNum(out, "maxQty", run.workload.maxQty, true);
  out << "  },\n";

  out << "  \"build\": {\n";
  writeKv(out, "compiler", meta.compilerId, false);
  writeKv(out, "compilerVersion", meta.compilerVersion, false);
  writeKv(out, "buildType", meta.buildType, false);
  writeKv(out, "buildFlags", meta.buildFlags, false);
  writeKv(out, "cxxStandard", meta.cxxStandard, false);
  writeKv(out, "gitCommit", meta.gitCommit, true);
  out << "  },\n";

  out << "  \"system\": {\n";
  writeKv(out, "hostname", meta.hostname, false);
  writeKv(out, "cpuModel", meta.cpuModel, false);
  writeKv(out, "cpuGovernor", meta.cpuGovernor, true);
  out << "  },\n";

  out << "  \"raw\": {\n";
  writeKv(out, "file", rawFile, false);
  writeKvNum(out, "recordCount", recordCount, false);
  writeKvNum(out, "recordSizeBytes", recordSizeBytes, false);
  writeKv(out, "format",
          "header{char magic[8]; uint32 version; uint32 recordSize; uint64 "
          "recordCount} then records{int64 latencyNs; uint8 eventType; uint8 "
          "pad[7]}",
          true);
  out << "  }\n";

  out << "}\n";
}
