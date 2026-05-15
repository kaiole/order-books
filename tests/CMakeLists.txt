include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(${PROJECT_NAME}_tests
               ${TEST_SOURCES}
               ${CMAKE_SOURCE_DIR}/bench/workload/ghost_book.cpp)

target_include_directories(${PROJECT_NAME}_tests
                           PRIVATE ${CMAKE_SOURCE_DIR}/bench)

target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}
                                                    GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)
