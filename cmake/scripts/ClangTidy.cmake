set(CMAKE_OPTIONS -DENABLE_CLANG_TIDY=ON)
set(CTEST_CONFIGURATION_TYPE Debug)

include(${CTEST_SOURCE_DIRECTORY}/cmake/scripts/Test.cmake)

ctest_submit()
