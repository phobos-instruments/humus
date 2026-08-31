# The boundary packs compile against; deliberately host-free (no io/, no gui/).
add_library(hum_sdk STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/Transport.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/Organism.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/LiveWavWriter.cpp
)
target_include_directories(hum_sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include)
target_compile_features(hum_sdk PUBLIC cxx_std_17)
target_link_libraries(hum_sdk PUBLIC
  juce::juce_core
  juce::juce_audio_basics
  juce::juce_audio_formats
)
# PUBLIC so host and packs compile JUCE identically; per-flag rationale in
# docs/dev/build.md, "The SDK boundary and the shared JUCE config".
target_compile_definitions(hum_sdk PUBLIC
  JUCE_USE_CURL=0
  JUCE_WEB_BROWSER=0
  JUCE_USE_FLAC=1
  JUCE_USE_OGGVORBIS=1
  JUCE_USE_MP3AUDIOFORMAT=1
  JUCE_PLUGINHOST_VST3=1
  JUCE_PLUGINHOST_LV2=1
  JUCE_USE_CAMERA=1
  JUCE_CATCH_UNHANDLED_EXCEPTIONS=1
)
if(APPLE)
  target_compile_definitions(hum_sdk PUBLIC JUCE_PLUGINHOST_AU=1)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Compile-time optional; JUCE dlopens libjack at runtime, ALSA remains.
  find_path(HUM_JACK_INCLUDE_DIR jack/jack.h)
  if(HUM_JACK_INCLUDE_DIR)
    target_compile_definitions(hum_sdk PUBLIC JUCE_JACK=1)
  else()
    message(STATUS "jack/jack.h not found - no JACK backend "
                   "(install libjack-jackd2-dev and re-configure to get one)")
  endif()
endif()
if(WIN32)
  # Vendored GPLv3 SDK; licensing in packaging/README.md § ASIO and
  # docs/dev/build.md. HUM_ENABLE_ASIO=OFF removes the GPLv3 component.
  option(HUM_ENABLE_ASIO "Build the ASIO backend (vendored GPLv3 SDK)" ON)
  if(HUM_ENABLE_ASIO)
    if(NOT HUM_ASIO_SDK_DIR AND DEFINED ENV{HUM_ASIO_SDK_DIR})
      set(HUM_ASIO_SDK_DIR "$ENV{HUM_ASIO_SDK_DIR}" CACHE PATH "Steinberg ASIO SDK root")
    endif()
    # NO_DEFAULT_PATH: a stray SDK on the machine must not pick itself.
    find_path(HUM_ASIO_INCLUDE_DIR iasiodrv.h
      HINTS "${HUM_ASIO_SDK_DIR}" "${HUM_ASIO_SDK_DIR}/common"
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio_sdk" NO_DEFAULT_PATH)
    if(HUM_ASIO_INCLUDE_DIR)
      target_compile_definitions(hum_sdk PUBLIC JUCE_ASIO=1)
      target_include_directories(hum_sdk PUBLIC "${HUM_ASIO_INCLUDE_DIR}")
      message(STATUS "ASIO backend enabled - ${HUM_ASIO_INCLUDE_DIR}")
    else()
      message(FATAL_ERROR "HUM_ENABLE_ASIO is ON but iasiodrv.h was not found. "
        "engine/third_party/asio_sdk should hold it - re-clone, point "
        "HUM_ASIO_SDK_DIR at an SDK copy, or configure with -DHUM_ENABLE_ASIO=OFF.")
    endif()
  else()
    message(STATUS "ASIO backend disabled - WASAPI exclusive remains available.")
  endif()
endif()
