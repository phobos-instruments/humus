# The build identity: which source tree a binary came from.
#
# project(Humus VERSION ...) stays the ONE version source and the ONE thing
# the hub compares. This is the other half of the answer - the tag, how far
# past it, the commit, and whether the tree was clean when it was built. It
# is deliberately NOT part of the version string: both comparators strip
# non-digits per dotted fragment, so '0.1.0+37.g95b8c21' would parse as
# [0,1,37,95821] and report itself newer than every release.
#
# Run at BUILD time, not configure time: a configure-time id goes stale the
# moment you commit, and a build id that lies is worse than none. The write
# goes through copy_if_different so an unchanged id costs no relink.

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

# No .git: this is the source zip. `git archive` substitutes the stamp on the
# way out (see .gitattributes export-subst), so the corresponding source of an
# official build still knows its commit. In a checkout the file holds the
# literal placeholder, which is not an answer.
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
