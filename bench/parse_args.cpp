#include "parse_args.h"

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

Args parseArgs(int argc, char** argv) {
  Args args;

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
      args.impl = parseImpl(value);
    } else if (flag == workloadSizeFlag) {
      args.workloadSize = parseNum<std::size_t>(value);
    } else if (flag == warmupSizeFlag) {
      args.warmupSize = parseNum<std::size_t>(value);
    } else if (flag == seedFlag) {
      args.seed = parseNum<std::uint64_t>(value);
    } else {
      throw std::invalid_argument("unknown arg: " + std::string(token));
    }
  }

  return args;
}
