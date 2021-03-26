include(GNUInstallDirs)
set(CMAKE_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/bongo)

add_library(bongo STATIC
  context.cpp
  detail/chan.cpp
  select.cpp
  sync.cpp)

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
  NAMESPACE Exegy::
  FILE bongo-targets.cmake)
