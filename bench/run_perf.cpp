#include "run_perf.h"

#include "parse_args.h"

#include <cstdio>
#include <string>
#include <sys/types.h>
#include <unistd.h>

namespace {

std::string getPerfOutputPath(Impl impl) {
  std::string fileName;

  switch (impl) {
  case Impl::MapDeque:
    fileName = "./tmp/map_deque.perf";
    break;
  case Impl::MapList:
    fileName = "./tmp/map_list.perf";
    break;
  }

  return fileName;
}

} // namespace

void runPerf(Impl impl) {
  pid_t pid = fork();
  if (pid == 0) {
    const std::string perfOutputPath = getPerfOutputPath(impl);
    const auto parentPid = std::to_string(getppid());

    execlp("perf", "perf", "record", "-F", "4000", "--call-graph", "fp", "-p",
           parentPid.c_str(), "-o", perfOutputPath.c_str(),
           static_cast<char*>(nullptr));

    std::perror("execlp");
    std::_Exit(1);
  }
}
