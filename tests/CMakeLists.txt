cmake_minimum_required(VERSION 3.20)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Test executables
add_executable(combinator_test combinator_test.cpp)
target_link_libraries(combinator_test PRIVATE http_parser gtest_main)
add_test(NAME CombinatorTest COMMAND combinator_test)

add_executable(http_parser_test http_parser_test.cpp)
target_link_libraries(http_parser_test PRIVATE http_parser gtest_main)
add_test(NAME HttpParserTest COMMAND http_parser_test)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(combinator_test)
gtest_discover_tests(http_parser_test)
