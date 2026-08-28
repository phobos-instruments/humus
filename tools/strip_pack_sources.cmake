# Usage: cmake -DDIR=<staged pack dir> -P tools/strip_pack_sources.cmake
#
# codesign refuses to seal a bundle with a pack's sources still in it: "code
# object is not signed at all - In subcomponent: .../packs/core/organisms/Bus/Bus.h".
# By extension, not by directory: organisms/ holds the manifests and editor
# blueprints too, and deleting the folder leaves the app with no parameters.
if(NOT DIR)
  message(FATAL_ERROR "strip_pack_sources.cmake: DIR is required")
endif()
file(GLOB_RECURSE HUM_PACK_SOURCES
     "${DIR}/*.cpp" "${DIR}/*.h" "${DIR}/*.hpp" "${DIR}/*.mm" "${DIR}/CMakeLists.txt")
foreach(f ${HUM_PACK_SOURCES})
  file(REMOVE "${f}")
endforeach()
