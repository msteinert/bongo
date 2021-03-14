add_library(bongo STATIC
  chan.cpp
  context.cpp
  sync.cpp)

target_compile_options(bongo
  PRIVATE
    -Wall
    -Wextra)

target_include_directories(bongo
  PUBLIC
    ${PROJECT_SOURCE_DIR})

target_link_libraries(bongo
  PUBLIC
    Threads::Threads)
