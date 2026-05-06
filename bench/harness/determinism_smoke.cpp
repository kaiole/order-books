#include "config.h"
#include "workload/event.h"
#include "workload/event_generator.h"

#include <cstring>
#include <iostream>

int main() {
  WorkloadConfig conf;
  conf.eventCount = 10'000;
  conf.warmupCount = 1'000;
  conf.seed = 67;
  conf.mix = EventMix{0.6, 0.2, 0.2};

  EventGenerator a(conf);
  EventGenerator b(conf);

  Workload workloadA = a.generate();
  Workload workloadB = b.generate();

  if (workloadA.size() != workloadB.size()) {
    std::cerr << "size mismatch: " << workloadA.size() << " vs "
              << workloadB.size() << "\n";
    return 1;
  }

  for (std::size_t i = 0; i < workloadA.size(); ++i) {
    if (std::memcmp(&workloadA[i], &workloadB[i], sizeof(Event)) != 0) {
      std::cerr << "event mismatch at index " << i << "\n";
      return 1;
    }
  }

  std::cout << "deterministic: " << workloadA.size() << " events match\n";
  return 0;
}
