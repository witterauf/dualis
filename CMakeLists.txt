cmake_minimum_required(VERSION 3.15)
project(dualis LANGUAGES CXX VERSION 0.1)

option(DUALIS_BUILD_TESTS OFF)
option(DUALIS_BUILD_EXAMPLES OFF)
option(DUALIS_ENABLE_INSTALL ON)

add_library(dualis INTERFACE)
target_include_directories(dualis
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
    $<INSTALL_INTERFACE:include/>
)
target_compile_features(dualis INTERFACE cxx_std_23)
add_library(dualis::dualis ALIAS dualis)

set(DUALIS_HEADERS
  src/dualis.h
  src/concepts.h
  src/containers.h
  src/containers_impl.h
  src/packing.h
  src/streams.h
  src/utilities.h
)

set_target_properties(dualis PROPERTIES PUBLIC_HEADER "${DUALIS_HEADERS}")

if(DUALIS_ENABLE_INSTALL)
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/dualisConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
  )

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dualisConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dualisConfig.cmake"
    INSTALL_DESTINATION lib/cmake/dualis
  )

  install(TARGETS dualis
    EXPORT dualis
    PUBLIC_HEADER DESTINATION include/
  )
  install(EXPORT dualis DESTINATION lib/cmake/dualis)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dualisConfigVersion.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/dualisConfig.cmake"
          DESTINATION lib/cmake/dualis
  )
endif()

if(DUALIS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(DUALIS_BUILD_TESTS)
  enable_testing()
  include(cmake/get_cpm.cmake)
  add_subdirectory(test)
endif()
