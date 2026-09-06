cmake_minimum_required(VERSION 3.22)

# Before project(), and tested by value: CMake pre-seeds an empty cache entry.
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

project(Humus VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HUM_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(HUM_BUILD_ID_H "${HUM_GENERATED_DIR}/HumBuildId.h")
set(HUM_BUILD_ID_ARGS
  -DVERSION=${PROJECT_VERSION}
  -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/..
  -DSTAMP=${CMAKE_CURRENT_SOURCE_DIR}/build-stamp.txt
  -DOUT=${HUM_BUILD_ID_H})
file(MAKE_DIRECTORY "${HUM_GENERATED_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} ${HUM_BUILD_ID_ARGS}
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildId.cmake)
add_custom_target(hum_build_id ALL
  COMMAND ${CMAKE_COMMAND} ${HUM_BUILD_ID_ARGS}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildId.cmake
  BYPRODUCTS ${HUM_BUILD_ID_H}
  COMMENT "Stamping the build id")

if(MSVC)
  add_compile_definitions(_USE_MATH_DEFINES NOMINMAX)
  add_compile_options(/MP)
  # One global core budget for cl.exe, not /MP times MSBuild's own parallelism.
  set(CMAKE_VS_GLOBALS "UseMultiToolTask=true" "EnforceProcessCountAcrossBuilds=true")
endif()
# Pack static libs also link into .humpack shared libs.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(HUM_DYN_PACKS "Also build packs as dynamic .humpack payloads" ON)

set(HUM_CODESIGN_IDENTITY "-" CACHE STRING
    "codesign identity: '-' = ad-hoc (dev), or 'Developer ID Application: ...'")

set(HUM_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk")
include(${HUM_SDK_DIR}/cmake/HumPack.cmake)

option(HUM_LIVE_AUDIO "Build live audio output (needs ALSA dev headers)" ON)
include(cmake/Juce.cmake)
include(cmake/HandsModel.cmake)
include(cmake/PoseModel.cmake)
include(cmake/CoreMLModels.cmake)
include(cmake/Sdk.cmake)
include(cmake/Vendored.cmake)
include(cmake/Packs.cmake)
include(cmake/Core.cmake)
include(cmake/Cli.cmake)
include(cmake/Tests.cmake)
include(cmake/Plugin.cmake)

enable_testing()
if(HUM_TESTS)
  add_test(NAME hum_tests COMMAND hum_tests)
endif()
include(cmake/GuiSources.cmake)
include(cmake/Gui.cmake)
include(cmake/Snapshot.cmake)
