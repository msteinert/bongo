add_executable(bongo-test
  chan.cpp
  context.cpp
  main.cpp
  sync.cpp
  time.cpp)

target_compile_options(bongo-test
  PRIVATE
    -Wall
    -Wextra)

target_compile_definitions(bongo-test
  PRIVATE
    -DCATCH_CONFIG_ENABLE_BENCHMARKING)

target_link_libraries(bongo-test
  PRIVATE
    Catch2::Catch2
    bongo)

add_test(
  NAME bongo-test
  COMMAND bongo-test)
