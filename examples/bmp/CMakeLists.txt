project(dualis-examples-bmp)

add_executable(dualis-bmp
  main.cc
)
target_link_libraries(dualis-bmp PRIVATE dualis::dualis)
target_compile_options(dualis-bmp
  INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)
