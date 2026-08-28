# Usage: cmake -DDIR=<staged dir> -P tools/strip_junk.cmake
if(NOT DIR)
  message(FATAL_ERROR "strip_junk.cmake: DIR is required")
endif()
file(GLOB_RECURSE HUM_JUNK "${DIR}/.DS_Store" "${DIR}/*/.DS_Store")
foreach(f ${HUM_JUNK})
  file(REMOVE "${f}")
endforeach()
