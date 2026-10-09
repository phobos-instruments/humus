# Usage: cmake -DDIR=<staged dir> -P tools/strip_junk.cmake
#
# Finder's .DS_Store files, then every folder left empty, deepest first: git never
# tracks an empty folder, so one in a staged tree is a leftover (a moved file's old
# home, sources stripped out of it) and the bundle mirrors the repository without it.
if(NOT DIR)
  message(FATAL_ERROR "strip_junk.cmake: DIR is required")
endif()
file(GLOB_RECURSE HUM_JUNK "${DIR}/.DS_Store" "${DIR}/*/.DS_Store")
foreach(f ${HUM_JUNK})
  file(REMOVE "${f}")
endforeach()
file(GLOB_RECURSE HUM_STAGED LIST_DIRECTORIES true "${DIR}/*")
set(HUM_STAGED_DIRS "")
foreach(p ${HUM_STAGED})
  if(IS_DIRECTORY "${p}")
    list(APPEND HUM_STAGED_DIRS "${p}")
  endif()
endforeach()
# Descending order puts every folder before its parent, so emptying a child is
# seen when the parent's turn comes.
list(SORT HUM_STAGED_DIRS ORDER DESCENDING)
foreach(d ${HUM_STAGED_DIRS})
  file(GLOB HUM_CHILDREN LIST_DIRECTORIES true "${d}/*")
  if(NOT HUM_CHILDREN)
    file(REMOVE_RECURSE "${d}")
  endif()
endforeach()
