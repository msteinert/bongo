set(CTEST_USE_LAUNCHERS 1)
set(ENV{CTEST_USE_LAUNCHERS_DEFAULT} 1)
set(CTEST_OUTPUT_ON_FAILURE 1)
set(CTEST_CMAKE_GENERATOR Ninja)

include(${CTEST_SOURCE_DIRECTORY}/cmake/Modules/LsbRelease.cmake)
lsb_release("-ds" CTEST_BUILD_NAME)

if(DEFINED ENV{GITLAB_CI})
  set(CTEST_SITE $ENV{CI_RUNNER_DESCRIPTION})
  set(DASHBOARD Continuous)
  file(WRITE ${CTEST_BINARY_DIRECTORY}/build_notes.txt
    "https://engrepo.exegy.net/$ENV{CI_PROJECT_PATH}/-/jobs/$ENV{CI_JOB_ID}")
  set(CTEST_NOTES_FILES ${CTEST_BINARY_DIRECTORY}/build_notes.txt)
else()
  site_name(CTEST_SITE)
  set(DASHBOARD Experimental)
endif()

ctest_read_custom_files(${CTEST_SOURCE_DIRECTORY}/cmake/scripts)
ctest_start(${DASHBOARD})
ctest_configure(OPTIONS "${CMAKE_OPTIONS}")
ctest_build()
ctest_test()
