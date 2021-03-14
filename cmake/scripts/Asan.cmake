set(CMAKE_OPTIONS -DENABLE_ASAN=ON)
set(CTEST_CONFIGURATION_TYPE Debug)
set(CTEST_MEMORYCHECK_TYPE "AddressSanitizer")

include(${CTEST_SOURCE_DIRECTORY}/cmake/scripts/Test.cmake)

ctest_memcheck()
ctest_submit()
