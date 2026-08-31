# Which source tree a binary came from; deliberately not part of the version
# string, and run at build time, never configure (docs/dev/build.md,
# "The build id").

set(BUILD_ID "")

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${SRC_DIR}/.git")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --always --dirty
    WORKING_DIRECTORY "${SRC_DIR}"
    OUTPUT_VARIABLE BUILD_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE git_result)
  if(NOT git_result EQUAL 0)
    set(BUILD_ID "")
  endif()
endif()

# No .git = the source zip; git archive substituted the stamp on the way out.
if(BUILD_ID STREQUAL "" AND EXISTS "${STAMP}")
  file(READ "${STAMP}" stamped)
  string(STRIP "${stamped}" stamped)
  if(NOT stamped MATCHES "^\\$Format")
    string(SUBSTRING "${stamped}" 0 8 short)
    set(BUILD_ID "g${short}")
  endif()
endif()

if(BUILD_ID STREQUAL "")
  set(BUILD_ID "unknown")
endif()

file(WRITE "${OUT}.tmp"
     "#pragma once\n"
     "#define HUM_VERSION \"${VERSION}\"\n"
     "#define HUM_BUILD_ID \"${BUILD_ID}\"\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${OUT}.tmp" "${OUT}")
file(REMOVE "${OUT}.tmp")
