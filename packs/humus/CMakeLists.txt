# builtin = linked into the app; addon = shipped only as a .humpack;
# tutorial = built dynamically, never linked.
set(HUM_PACK_ROLE builtin PARENT_SCOPE)

# Humus organism pack - original organisms (currently the DJ Deck).
# Carries its own third-party deps (time-stretch engines for keylock).
include(FetchContent)

file(GLOB HUM_HUMUS_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/organisms/*/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp)

add_library(hum_pack_humus STATIC
  register.cpp
  layouts.cpp
  ${HUM_HUMUS_SOURCES}
)
target_include_directories(hum_pack_humus PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/organisms   # "<Name>/<Name>.h"
  ${CMAKE_CURRENT_SOURCE_DIR})               # "common/TimeStretcher.h"
target_include_directories(hum_pack_humus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../engine/src)
target_link_libraries(hum_pack_humus PUBLIC
  hum_sdk
  msfa                    # pH: six-operator FM engine (vendored, Apache-2.0)
  nuked_opn2              # pH: OPN2 chip core (vendored, LGPL-2.1)
  nuked_opm               # pH: OPM chip core (vendored, LGPL-2.1)
  nuked_opl3              # pH: OPL3 chip core (vendored, LGPL-2.1)
  resid                   # Silt: SID chip core (vendored, GPL-2.0-or-later)
  nsfplay_sound           # Grit: Famicom APU + expansion audio (vendored, NSFPlay)
  juce::juce_core
  juce::juce_audio_basics
  juce::juce_audio_formats
  juce::juce_dsp          # SoundSpace corpus analysis (FFT features)
  juce::juce_graphics     # Halogen: decoding a picture into a spectrogram
  juce::juce_video        # CameraIn camera capture (AVFoundation-backed)
)


# ---- Deck keylock (time-stretch) ----
# Default engine: Signalsmith Stretch (MIT, header-only) - high quality. Fallback:
# SoundTouch (LGPL). Fetched at configure time (needs network on the first build).
# Turn off with -DHUM_KEYLOCK=OFF (Keylock then falls back to resampling); switch
# engines with -DHUM_KEYLOCK_ENGINE=soundtouch.
option(HUM_KEYLOCK "Build deck keylock time-stretch (needs network at configure)" ON)
set(HUM_KEYLOCK_ENGINE "signalsmith" CACHE STRING "keylock engine: signalsmith | soundtouch")
if(HUM_KEYLOCK)
  if(HUM_KEYLOCK_ENGINE STREQUAL "soundtouch")
    set(SOUNDSTRETCH OFF CACHE BOOL "" FORCE)     # don't build the soundstretch CLI app
    set(SOUNDTOUCH_DLL OFF CACHE BOOL "" FORCE)
    set(INTEGER_SAMPLES OFF CACHE BOOL "" FORCE)   # float sample path
    FetchContent_Declare(soundtouch
      GIT_REPOSITORY https://gitlab.com/soundtouch/soundtouch.git
      GIT_TAG 2.3.1 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(soundtouch)
    target_link_libraries(hum_pack_humus PUBLIC SoundTouch)
    target_compile_definitions(hum_pack_humus PUBLIC HUM_HAS_KEYLOCK=1 HUM_STRETCH_SOUNDTOUCH=1)
  else()
    # Header-only: fetch sources but skip their CMake (it needs cmake >= 3.24) by
    # pointing SOURCE_SUBDIR at a non-existent dir, then add the include paths.
    FetchContent_Declare(signalsmith_stretch
      GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
      GIT_TAG 1.1.0 GIT_SHALLOW TRUE SOURCE_SUBDIR _no_build)
    FetchContent_Declare(signalsmith_linear
      GIT_REPOSITORY https://github.com/Signalsmith-Audio/linear.git
      GIT_TAG 0.3.1 GIT_SHALLOW TRUE SOURCE_SUBDIR _no_build)
    FetchContent_MakeAvailable(signalsmith_stretch signalsmith_linear)
    target_include_directories(hum_pack_humus SYSTEM PUBLIC
      ${signalsmith_stretch_SOURCE_DIR}
      ${signalsmith_linear_SOURCE_DIR}/include)
    target_compile_definitions(hum_pack_humus PUBLIC HUM_HAS_KEYLOCK=1 HUM_STRETCH_SIGNALSMITH=1)
  endif()
endif()

# Dynamic build (the .humpack payload): the same static lib + the C entry points.
if(HUM_DYN_PACKS)
  hum_add_pack_dyn(humus)
  hum_pack_bundle(humus)
endif()
