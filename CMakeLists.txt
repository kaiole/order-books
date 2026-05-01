cmake_minimum_required(VERSION 3.20)
project(
  order-books
  VERSION 0.0.0
  DESCRIPTION "Benchmarks for order book implementations"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

option(ORDER_BOOKS_BUILD_TESTS "Build tests" ON)
option(ORDER_BOOKS_BUILD_BENCH "Build benchmarks" ON)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(${PROJECT_NAME} ${LIB_SOURCES})

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include>)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

if(ORDER_BOOKS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(ORDER_BOOKS_BUILD_BENCH)
  add_subdirectory(bench)
endif()
