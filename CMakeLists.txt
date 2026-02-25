cmake_minimum_required(VERSION 3.22)

project(sketch-hse)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/BuildFlags.cmake)
include(cmake/FindBenchmark.cmake)
include(cmake/FindGTest.cmake)
include(cmake/Task.cmake)

add_subdirectory(01-intro)
