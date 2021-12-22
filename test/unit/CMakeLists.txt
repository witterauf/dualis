add_executable(dualis-unit-tests
  containers.cc
  rw.cc
)
target_link_libraries(dualis-unit-tests
  PRIVATE
    dualis::dualis
    Catch2::Catch2WithMain
)
target_compile_options(dualis-unit-tests
  INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)
add_test(NAME dualis-unit-tests COMMAND dualis-unit-tests)