cmake_minimum_required(VERSION 3.22)

project(sketch-hse)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

include(cmake/BuildFlags.cmake)
include(cmake/FindBenchmark.cmake)
include(cmake/FindGTest.cmake)
include(cmake/Task.cmake)

add_subdirectory(01-intro)
add_subdirectory(02-distinct)
add_subdirectory(03-frequent-items)
add_subdirectory(04-quantiles)
add_subdirectory(05-sampling)
add_subdirectory(06-moments)
