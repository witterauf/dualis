add_executable(dualis-test-containers
  containers.cc
  matchers.h
  mock_allocator.h
)
target_link_libraries(dualis-test-containers
  PRIVATE
    dualis::dualis
    Catch2::Catch2WithMain
)
target_compile_options(dualis-test-containers
  INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)
add_test(NAME dualis-test-containers COMMAND dualis-test-containers)