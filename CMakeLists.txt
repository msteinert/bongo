cmake_minimum_required(VERSION 3.12)
project(bongo
  DESCRIPTION "Go APIs ported to C++20"
  LANGUAGES CXX)

option(ENABLE_CLANG_TIDY "Enable clang-tidy checks"       OFF)
option(ENABLE_COVERAGE   "Enable code coverage reporting" OFF)
option(ENABLE_ASAN       "Enable the address sanitizer"   OFF)
option(ENABLE_TSAN       "Enable the thread sanitizer"    OFF)
option(WITH_EXAMPLES     "Build the example applications" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options("-fdiagnostics-color")
endif()

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
find_package(Catch2)

include(CTest)
add_subdirectory(bongo)
if(WITH_EXAMPLES)
  add_subdirectory(examples)
endif()
