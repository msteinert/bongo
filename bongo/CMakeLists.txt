include(GNUInstallDirs)
set(CMAKE_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/bongo)

add_library(bongo STATIC
  bytes/error.cpp
  bytes/buffer.cpp
  bytes/reader.cpp
  context/context.cpp
  context/error.cpp
  crypto/sha1/error.cpp
  crypto/sha1/sha1.cpp
  detail/poll/error.cpp
  detail/poll/fd_unix.cpp
  detail/syscall/unix.cpp
  fmt/detail/fmt.cpp
  fmt/detail/printer.cpp
  io/error.cpp
  io/pipe.cpp
  os/file.cpp
  os/file_unix.cpp
  os/os.cpp
  runtime/detail/chan_impl.cpp
  runtime/select.cpp
  strconv/error.cpp
  strings/builder.cpp
  strings/error.cpp
  strings/reader.cpp
  strings/strings.cpp
  sync/wait_group.cpp)

target_compile_definitions(bongo
  PRIVATE
    -D_FILE_OFFSET_BITS=64)

target_compile_options(bongo
  PRIVATE
    -Wall
    -Wextra)

target_include_directories(bongo
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(bongo
  PUBLIC
    $<BUILD_INTERFACE:config-asan>
    $<BUILD_INTERFACE:config-coverage>
    $<BUILD_INTERFACE:config-tsan>
    Threads::Threads)

if(TARGET Catch2::Catch2)
  add_executable(bongo-test
    bytes/buffer_test.cpp
    bytes/bytes_test.cpp
    bytes/reader_test.cpp
    context/context_test.cpp
    crypto/sha1/sha1_test.cpp
    detail/poll/fd_mutex_test.cpp
    fmt/fmt_test.cpp
    io/fs/fs_test.cpp
    io/io_test.cpp
    io/pipe_test.cpp
    io/multi_test.cpp
    main_test.cpp
    os/os_test.cpp
    runtime/chan_test.cpp
    strconv/atob_test.cpp
    strconv/atoc_test.cpp
    strconv/atof_test.cpp
    strconv/atoi_test.cpp
    strconv/ctoa_test.cpp
    strconv/detail/decimal_test.cpp
    strconv/ftoa_test.cpp
    strconv/itoa_test.cpp
    strings/builder_test.cpp
    strings/reader_test.cpp
    strings/strings_test.cpp
    sync/wait_group_test.cpp
    time/timer_test.cpp
    unicode/utf16/utf16_test.cpp
    unicode/utf8/utf8_test.cpp)

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
endif()

install(
  TARGETS bongo
  EXPORT bongo
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
    PATTERN *.h)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  bongo-config.cmake.in bongo-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
  PATH_VARS
    CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_CONFIGDIR)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/bongo-config.cmake
  DESTINATION ${CMAKE_INSTALL_CONFIGDIR})

install(
  EXPORT bongo
  DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
  NAMESPACE Bongo::
  FILE bongo-targets.cmake)
