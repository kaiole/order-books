#include "parse_args.h"

#include "types.h"

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

  throw std::invalid_argument("unknown impl: " + std::string(value));
}

} // namespace

BenchConfig parseArgs(int argc, char** argv) {
  BenchConfig config;
  bool implSet = false;
  bool addRatioSet = false;
  bool cancelRatioSet = false;
  bool modifyRatioSet = false;

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
      config.eventCount = parseNum<std::size_t>(value);
    } else if (flag == warmupSizeFlag) {
      config.warmupCount = parseNum<std::size_t>(value);
    } else if (flag == seedFlag) {
      config.seed = parseNum<std::uint64_t>(value);
    } else if (flag == addRatioFlag) {
      config.addRatio = parseDouble(value);
      addRatioSet = true;
    } else if (flag == cancelRatioFlag) {
      config.cancelRatio = parseDouble(value);
      cancelRatioSet = true;
    } else if (flag == modifyRatioFlag) {
      config.modifyRatio = parseDouble(value);
      modifyRatioSet = true;
    } else {
      throw std::invalid_argument("unknown arg: " + std::string(token));
    }
  }

  if (!implSet || !addRatioSet || !cancelRatioSet || !modifyRatioSet) {
    throw std::invalid_argument("missing required args\n"
                                "usage: "
                                "--impl=<MapDeque|MapList> "
                                "--add-ratio=<double> "
                                "--cancel-ratio=<double> "
                                "--modify-ratio=<double> "
                                "[--size=<n>] [--warmup=<n>] [--seed=<n>]");
  }

  return config;
}
