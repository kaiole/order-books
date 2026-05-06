include(FetchContent)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.1)

set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(googlebenchmark)

execute_process(
  COMMAND git rev-parse --short=12 HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE BENCH_GIT_COMMIT
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT BENCH_GIT_COMMIT)
  set(BENCH_GIT_COMMIT "unknown")
endif()

set(BENCH_COMMON_SOURCES
    workload/event_generator.cpp output/run_metadata.cpp runner/run_perf.cpp
    harness/parse_args.cpp output/write_latencies.cpp)

add_library(${PROJECT_NAME}_bench_common STATIC ${BENCH_COMMON_SOURCES})

target_include_directories(${PROJECT_NAME}_bench_common
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}_bench_common PUBLIC ${PROJECT_NAME}
                                                          benchmark::benchmark)

set(BENCH_COMPILE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -O3
    -g
    -DNDEBUG
    -march=native
    -fno-omit-frame-pointer)

set(BENCH_BUILD_FLAGS_STRING "")
foreach(opt ${BENCH_COMPILE_OPTIONS})
  set(BENCH_BUILD_FLAGS_STRING "${BENCH_BUILD_FLAGS_STRING} ${opt}")
endforeach()
string(STRIP "${BENCH_BUILD_FLAGS_STRING}" BENCH_BUILD_FLAGS_STRING)

target_compile_options(${PROJECT_NAME}_bench_common
                       PUBLIC ${BENCH_COMPILE_OPTIONS})

target_compile_definitions(
  ${PROJECT_NAME}_bench_common
  PUBLIC BENCH_GIT_COMMIT="${BENCH_GIT_COMMIT}"
         BENCH_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
         BENCH_BUILD_FLAGS="${BENCH_BUILD_FLAGS_STRING}")

add_executable(${PROJECT_NAME}_bench harness/main.cpp)
target_link_libraries(${PROJECT_NAME}_bench
                      PRIVATE ${PROJECT_NAME}_bench_common)

add_executable(${PROJECT_NAME}_gbench harness/gbench_main.cpp)
target_link_libraries(${PROJECT_NAME}_gbench
                      PRIVATE ${PROJECT_NAME}_bench_common)

add_executable(${PROJECT_NAME}_determinism_smoke harness/determinism_smoke.cpp)
target_link_libraries(${PROJECT_NAME}_determinism_smoke
                      PRIVATE ${PROJECT_NAME}_bench_common)

add_executable(${PROJECT_NAME}_microbench_clock microbench/clock_overhead.cpp)
target_link_libraries(${PROJECT_NAME}_microbench_clock
                      PRIVATE ${PROJECT_NAME}_bench_common)

add_test(NAME bench_determinism COMMAND ${PROJECT_NAME}_determinism_smoke)
add_test(NAME bench_gbench_smoke
         COMMAND ${PROJECT_NAME}_gbench --benchmark_min_time=1x
                 --benchmark_repetitions=1)
set_tests_properties(bench_gbench_smoke PROPERTIES TIMEOUT 60)

add_test(
  NAME bench_raw_smoke
  COMMAND
    ${PROJECT_NAME}_bench --impl=MapList --add-ratio=0.6 --modify-ratio=0.2
    --cancel-ratio=0.2 --size=10000 --warmup=1000
    --output-dir=${CMAKE_BINARY_DIR}/test_results)
set_tests_properties(bench_raw_smoke PROPERTIES TIMEOUT 30)
