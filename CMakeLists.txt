cmake_minimum_required(VERSION 3.15)
project(GMap VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# Shared sources (no main)
set(LIB_SOURCES src/Graph.cpp src/Algorithms.cpp)

# Main app
add_executable(gmap ${LIB_SOURCES} src/main.cpp)
target_include_directories(gmap PRIVATE include)

# Test binary
add_executable(gmap_tests ${LIB_SOURCES} tests/test_gmap.cpp)
target_include_directories(gmap_tests PRIVATE include tests)

# Scale testing binary
add_executable(scale_test ${LIB_SOURCES} tests/scale_test.cpp)
target_include_directories(scale_test PRIVATE include tests)

# Stress testing binary
add_executable(stress_test ${LIB_SOURCES} tests/stress_test.cpp)
target_include_directories(stress_test PRIVATE include tests)

enable_testing()
add_test(NAME GMapTests COMMAND gmap_tests)

message(STATUS "Targets: gmap | gmap_tests")

# Bottleneck analysis binary
add_executable(bottleneck ${LIB_SOURCES} tests/bottleneck.cpp)
target_include_directories(bottleneck PRIVATE include tests)