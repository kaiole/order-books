file(GLOB_RECURSE BENCH_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(${PROJECT_NAME}_bench ${BENCH_SOURCES})

target_link_libraries(${PROJECT_NAME}_bench PRIVATE ${PROJECT_NAME})

target_compile_options(
  ${PROJECT_NAME}_bench
  PRIVATE -Wall
          -Wextra
          -Wpedantic
          -O3
          -g
          -DNDEBUG
          -march=native
          -fno-omit-frame-pointer)
