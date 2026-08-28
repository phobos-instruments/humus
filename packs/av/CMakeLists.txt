# builtin = linked into the app; addon = shipped only as a .humpack;
# tutorial = built dynamically, never linked.
set(HUM_PACK_ROLE builtin PARENT_SCOPE)

# AV pack - audio-reactive visuals (Lumen; every future visual organism lands
# here). Pre-installed like core/humus/tribe: statically linked into the host,
# registered via BuiltinPacks. The GL window / video decode live GUI-side
# (engine/src/gui); the organisms here are the audio-thread half only, so the
# pack stays a plain SDK citizen.
file(GLOB HUM_AV_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/organisms/*/*.cpp)

add_library(hum_pack_av STATIC
  register.cpp
  ${HUM_AV_SOURCES}
)
target_include_directories(hum_pack_av PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/organisms)  # "<Name>/<Name>.h"
target_link_libraries(hum_pack_av PUBLIC
  hum_sdk
  juce::juce_core
  juce::juce_audio_basics
  juce::juce_video      # CameraIn capture (AVFoundation-backed); brings juce_graphics
)

# Dynamic build (the .humpack payload): the same static lib + the C entry points.
if(HUM_DYN_PACKS)
  hum_add_pack_dyn(av)
  hum_pack_bundle(av)
endif()
