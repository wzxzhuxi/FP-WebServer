cmake_minimum_required(VERSION 3.20)
project(function-tinywebserver CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/parser)

# Header-only parser library
add_library(http_parser INTERFACE)
target_include_directories(http_parser INTERFACE
    ${CMAKE_SOURCE_DIR}/parser
)

# Main server executable (when main.cpp exists)
if(EXISTS ${CMAKE_SOURCE_DIR}/main.cpp)
    add_executable(server main.cpp)
    target_link_libraries(server PRIVATE http_parser)
endif()

# Router example
if(EXISTS ${CMAKE_SOURCE_DIR}/example/router_usage.cpp)
    add_executable(router_example example/router_usage.cpp)
    target_include_directories(router_example PRIVATE ${CMAKE_SOURCE_DIR}/router)
    target_link_libraries(router_example PRIVATE http_parser)
endif()

# Tests
enable_testing()
add_subdirectory(tests)
