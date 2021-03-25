set(CMAKE_OPTIONS -DENABLE_COVERAGE=ON)
set(CTEST_CONFIGURATION_TYPE Debug)
set(CTEST_COVERAGE_COMMAND ${CTEST_SOURCE_DIRECTORY}/cmake/scripts/gcov.sh)

include(${CTEST_SOURCE_DIRECTORY}/cmake/scripts/Test.cmake)

ctest_coverage()
ctest_submit()
