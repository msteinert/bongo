cmake_minimum_required(VERSION 3.16)
project(bongo
  DESCRIPTION "C++ implementation of Golang interfaces"
  LANGUAGES CXX)

option(ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
option(ENABLE_ASAN "Enable the address sanitizer" OFF)
option(ENABLE_TSAN "Enable the thread sanitizer" OFF)
option(WITH_EXAMPLES "Build the example applications" ON)

# Check for local cmake modules before ${CMAKE_ROOT}/Modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
set(CMAKE_CXX_STANDARD 17)

# Configure clang-tidy
if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY NAMES clang-tidy)
  set(CLANG_TIDY_CHECKS
    -*
    bugprone-*
    cert-*
    clang-analyzer-*
    portability-*)
  list(JOIN CLANG_TIDY_CHECKS "," __clang_tidy_checks)
  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} -checks=${__clang_tidy_checks})
endif()

# Configure coverage
add_library(config-coverage INTERFACE)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(config-coverage
    INTERFACE
      --coverage -fprofile-abs-path)
  target_link_options(config-coverage
    INTERFACE
      --coverage)
endif()

# Configure ASAN
add_library(config-asan INTERFACE)
if(ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(config-asan
    INTERFACE
      -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(config-coverage
    INTERFACE
      -fsanitize=address)
endif()

# Configure TSAN
add_library(config-tsan INTERFACE)
if(ENABLE_TSAN AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(config-tsan
    INTERFACE
      -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(config-coverage
    INTERFACE
      -fsanitize=thread)
endif()

# Attempt to load Conan
# https://conan.io
include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

add_subdirectory(bongo)
if(WITH_EXAMPLES)
  add_subdirectory(examples)
endif()

# Configure testing
include(CTest)

if(BUILD_TESTING)
  find_package(Catch2)
  if(TARGET Catch2::Catch2)
    add_subdirectory(tests)
  endif()
endif()
