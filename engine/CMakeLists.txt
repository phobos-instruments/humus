cmake_minimum_required(VERSION 3.22)

# Before project(), and tested by value: CMake pre-seeds an empty cache entry.
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

project(Humus VERSION 0.4.0 LANGUAGES C CXX)

# A single-config generator with no build type is -O0 with live asserts, which
# xruns in a realtime app; a first-time builder following the README gets Release.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Release unless asked otherwise)" FORCE)
endif()

set(HUM_SANITIZE "" CACHE STRING "Sanitizer for the whole build: address, thread or undefined (empty for none)")
if(HUM_SANITIZE)
  add_compile_options(-fsanitize=${HUM_SANITIZE} -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=${HUM_SANITIZE})
endif()

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

# moduleinfo.json is VST3 3.7 metadata a host may read instead of loading the
# module to ask it. JUCE generates it by running juce_vst3_helper, which it has
# just built and not signed - and a Windows machine under Device Guard / WDAC
# refuses to run an unsigned binary, which fails the build rather than the step.
# Off, the plugin builds and hosts load it exactly as they did before 3.7 added
# the file. Declared here because Tests.cmake and Plugin.cmake both read it and
# Tests.cmake is included first.
option(HUM_VST3_MANIFEST "Generate the VST3 moduleinfo.json (runs a freshly built helper)" ON)
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
include(cmake/Warnings.cmake)
foreach(t hum_sdk hum_core hum hum_tests hum_snapshot hum_gui Humus
          hum_pack_core hum_pack_humus hum_pack_av hum_pack_audiomulch hum_pack_sample)
  hum_warnings(${t})
endforeach()

enable_testing()
if(HUM_TESTS)
  add_test(NAME hum_tests COMMAND hum_tests)
endif()
include(cmake/GuiSources.cmake)
include(cmake/Gui.cmake)
include(cmake/Snapshot.cmake)
