#include "parse_args.h"

#include "config.h"

#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view implFlag = "--impl";
constexpr std::string_view workloadSizeFlag = "--size";
constexpr std::string_view warmupSizeFlag = "--warmup";
constexpr std::string_view seedFlag = "--seed";
constexpr std::string_view addRatioFlag = "--add-ratio";
constexpr std::string_view cancelRatioFlag = "--cancel-ratio";
constexpr std::string_view modifyRatioFlag = "--modify-ratio";
constexpr std::string_view scenarioFlag = "--scenario";
constexpr std::string_view outputDirFlag = "--output-dir";
constexpr std::string_view maxPriceOffsetFlag = "--max-price-offset";
constexpr std::string_view crossProbFlag = "--cross-prob";
constexpr std::string_view gtcRatioFlag = "--gtc-ratio";
constexpr std::string_view iocRatioFlag = "--ioc-ratio";
constexpr std::string_view fokRatioFlag = "--fok-ratio";

double parseDouble(std::string_view value) {
  if (value.empty()) {
    throw std::invalid_argument("expected number");
  }

  double result{};
  const char* begin = value.data();
  const char* end = value.data() + value.size();

  auto [ptr, ec] = std::from_chars(begin, end, result);

  if (ec == std::errc::invalid_argument) {
    throw std::invalid_argument("invalid number: " + std::string(value));
  }

  if (ec == std::errc::result_out_of_range) {
    throw std::out_of_range("number out of range: " + std::string(value));
  }

  if (ptr != end) {
    throw std::invalid_argument("trailing characters in number: " +
                                std::string(value));
  }

  return result;
}

template <std::unsigned_integral T>
T parseNum(std::string_view value) {
  if (value.empty()) {
    throw std::invalid_argument("expected number");
  }

  T result{};
  const char* begin = value.data();
  const char* end = value.data() + value.size();

  auto [ptr, ec] = std::from_chars(begin, end, result);

  if (ec == std::errc::invalid_argument) {
    throw std::invalid_argument("invalid number: " + std::string(value));
  }

  if (ec == std::errc::result_out_of_range) {
    throw std::out_of_range("number out of range: " + std::string(value));
  }

  if (ptr != end) {
    throw std::invalid_argument("trailing characters in number: " +
                                std::string(value));
  }

  return result;
}

Impl parseImpl(std::string_view value) {
  if (value == "MapDeque" || value == "mapdeque") {
    return Impl::MapDeque;
  }

  if (value == "MapList" || value == "maplist") {
    return Impl::MapList;
  }

  if (value == "MapDequeIter" || value == "mapdequeiter") {
    return Impl::MapDequeIter;
  }

  if (value == "MapListIter" || value == "maplistiter") {
    return Impl::MapListIter;
  }

  if (value == "MapDequeFat" || value == "mapdequefat") {
    return Impl::MapDequeFat;
  }

  throw std::invalid_argument("unknown impl: " + std::string(value));
}

} // namespace

RunConfig parseArgs(int argc, char** argv) {
  RunConfig config;
  bool implSet = false;

  for (int i = 1; i < argc; ++i) {
    std::string_view token = argv[i];

    auto eq = token.find('=');
    if (eq == std::string_view::npos) {
      throw std::invalid_argument("expected --flag=value: " +
                                  std::string(token));
    }

    auto flag = token.substr(0, eq);
    auto value = token.substr(eq + 1);

    if (flag == implFlag) {
      config.impl = parseImpl(value);
      implSet = true;
    } else if (flag == workloadSizeFlag) {
      config.workload.eventCount = parseNum<std::size_t>(value);
    } else if (flag == warmupSizeFlag) {
      config.workload.warmupCount = parseNum<std::size_t>(value);
    } else if (flag == seedFlag) {
      config.workload.seed = parseNum<std::uint64_t>(value);
    } else if (flag == addRatioFlag) {
      config.workload.mix.add = parseDouble(value);
    } else if (flag == cancelRatioFlag) {
      config.workload.mix.cancel = parseDouble(value);
    } else if (flag == modifyRatioFlag) {
      config.workload.mix.modify = parseDouble(value);
    } else if (flag == scenarioFlag) {
      config.scenarioName = std::string(value);
    } else if (flag == outputDirFlag) {
      config.outputDir = std::string(value);
    } else if (flag == maxPriceOffsetFlag) {
      config.workload.maxPriceOffset =
          static_cast<Price>(parseNum<std::uint32_t>(value));
    } else if (flag == crossProbFlag) {
      config.workload.crossProbability = parseDouble(value);
    } else if (flag == gtcRatioFlag) {
      config.workload.tifMix.gtc = parseDouble(value);
    } else if (flag == iocRatioFlag) {
      config.workload.tifMix.ioc = parseDouble(value);
    } else if (flag == fokRatioFlag) {
      config.workload.tifMix.fok = parseDouble(value);
    } else {
      throw std::invalid_argument("unknown arg: " + std::string(token));
    }
  }

  if (!implSet) {
    throw std::invalid_argument(
        "missing required args\n"
        "usage: "
        "--impl=<MapDeque|MapList|MapDequeIter|MapListIter|MapDequeFat> "
        "[--add-ratio=<double>] [--cancel-ratio=<double>] "
        "[--modify-ratio=<double>] "
        "[--cross-prob=<double>] [--gtc-ratio=<double>] "
        "[--ioc-ratio=<double>] [--fok-ratio=<double>] "
        "[--size=<n>] [--warmup=<n>] [--seed=<n>] "
        "[--scenario=<name>] [--output-dir=<path>] "
        "[--max-price-offset=<n>]");
  }

  return config;
}
