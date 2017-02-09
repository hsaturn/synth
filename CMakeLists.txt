cmake_minimum_required (VERSION 2.8.11)
project(synth)

set(CMAKE_BUILD_TYPE Debug)

add_compile_options (-Wall -O4 -std=c++11)
add_subdirectory (lib)
add_subdirectory (bin)

