cmake_minimum_required(VERSION 3.16)
project(serwer C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
include_directories(src)

file(GLOB_RECURSE SOURCES src/*.c)
add_executable(serwer ${SOURCES})


SET(GCC_COVERAGE_COMPILE_FLAGS "-I/usr/include/ -pthread")
SET(GCC_COVERAGE_LINK_FLAGS    "-pthread")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")