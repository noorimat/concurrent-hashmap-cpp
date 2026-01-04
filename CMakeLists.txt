cmake_minimum_required(VERSION 3.20)
project(concurrent_hashmap_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -O2 -Wall -Wextra")

# Main library
add_library(lockfree_hashmap INTERFACE)
target_include_directories(lockfree_hashmap INTERFACE include)

# Demo executable
add_executable(demo src/main.cpp)
target_link_libraries(demo lockfree_hashmap pthread)

# Stress test executable
add_executable(stress_test src/stress_test.cpp)
target_link_libraries(stress_test lockfree_hashmap pthread)
# enable_testing()