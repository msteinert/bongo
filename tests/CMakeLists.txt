add_executable(bongo-test
  chan.cpp
  context.cpp
  main.cpp
  strings.cpp
  sync.cpp
  time.cpp
  unicode.cpp
  unicode/utf16.cpp
  unicode/utf8.cpp)

if(NOT ENABLE_CLANG_TIDY)
  target_compile_definitions(bongo-test
    PRIVATE
      -DCATCH_CONFIG_ENABLE_BENCHMARKING)
endif()

target_compile_options(bongo-test
  PRIVATE
    -Wall
    -Wextra)

target_link_libraries(bongo-test
  PRIVATE
    Catch2::Catch2
    bongo)

add_test(
  NAME bongo-test
  COMMAND bongo-test)
