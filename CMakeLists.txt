cmake_minimum_required(VERSION 3.15)
project(dualis LANGUAGES CXX VERSION 0.1)

option(DUALIS_BUILD_TESTS OFF)
option(DUALIS_BUILD_EXAMPLES OFF)

add_library(dualis INTERFACE)
target_include_directories(dualis INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(dualis INTERFACE cxx_std_20)
add_library(dualis::dualis ALIAS dualis)

if(DUALIS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(DUALIS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
