# builtin = linked into the app; addon = shipped only as a .humpack;
# tutorial = built dynamically, never linked.
set(HUM_PACK_ROLE builtin PARENT_SCOPE)

# Core organism pack - the plumbing (I/O, MIDI ports, gains, mixers, buses,
# matrices). Always statically linked into the host (pre-installed); the dynamic
# build exists only so the pack-sdk conformance suite exercises it.
file(GLOB HUM_CORE_PACK_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/organisms/*/*.cpp)

add_library(hum_pack_core STATIC
  register.cpp
  layouts.cpp
  ${HUM_CORE_PACK_SOURCES}
)
# Organisms include each other as "<Name>/<Name>.h".
target_include_directories(hum_pack_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/organisms)
# core/packs/Registry.h only (header-inline registration API; no host link dependency).
target_include_directories(hum_pack_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../engine/src)
target_link_libraries(hum_pack_core PUBLIC
  hum_sdk
  juce::juce_core
  juce::juce_audio_basics
  juce::juce_audio_formats
)

if(HUM_DYN_PACKS)
  hum_add_pack_dyn(core)
  hum_pack_bundle(core)
endif()
