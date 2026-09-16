# builtin = linked into the app; addon = shipped only as a .humpack;
# tutorial = built dynamically, never linked.
set(HUM_PACK_ROLE builtin PARENT_SCOPE)

# AV pack - audio-reactive visuals (Lumen; every future visual organism lands
# here). Pre-installed like core/humus: statically linked into the host,
# registered via BuiltinPacks. The GL window / video decode (AVFoundation on
# macOS, FFmpeg elsewhere, HAP everywhere) live GUI-side
# (engine/src/gui); the organisms here are the audio-thread half only, so the
# pack stays a plain SDK citizen.
file(GLOB HUM_AV_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/organisms/*/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp)

add_library(hum_pack_av STATIC
  register.cpp
  ${HUM_AV_SOURCES}
)
target_include_directories(hum_pack_av PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/organisms   # "<Name>/<Name>.h"
  ${CMAKE_CURRENT_SOURCE_DIR})               # "common/HumNet.h"
target_include_directories(hum_pack_av PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../engine/src)
target_link_libraries(hum_pack_av PUBLIC
  hum_sdk
  juce::juce_core
  juce::juce_audio_basics
  juce::juce_video      # CameraIn capture (AVFoundation-backed); brings juce_graphics
)

# Hands: the portable detector, compiled on every platform so the path Linux
# and Windows rely on cannot rot behind a mac-only build - and so macOS can run
# it against Vision on the same frame. It arrives through the source glob; the
# mac-only backends below are the Vision and Core ML halves.
if(APPLE)
  target_sources(hum_pack_av PRIVATE
    common/CoreMLNetMac.mm
    organisms/Hands/HandsVisionMac.mm
    organisms/Skeleton/BodyCoreMLMac.mm)
  target_link_libraries(hum_pack_av PUBLIC
    "-framework Vision" "-framework CoreGraphics"
    "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo"
    "-framework CoreML")
endif()

# Dynamic build (the .humpack payload): the same static lib + the C entry points.
if(HUM_DYN_PACKS)
  hum_add_pack_dyn(av)
  hum_pack_bundle(av)
endif()
