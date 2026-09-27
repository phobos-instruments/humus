# JUCE compiled once, into hum_juce, instead of once per target; hum_juce_modal
# is the same with modal loops on, for the tests and the snapshot tool
# (docs/dev/build.md, "JUCE, compiled once").
set(HUM_JUCE_MODULES
  juce::juce_core
  juce::juce_events
  juce::juce_data_structures
  juce::juce_audio_basics
  juce::juce_audio_formats
  juce::juce_audio_processors
  juce::juce_audio_devices
  juce::juce_audio_utils
  juce::juce_dsp
  juce::juce_osc
  juce::juce_graphics
  juce::juce_gui_basics
  juce::juce_gui_extra
  juce::juce_video
  juce::juce_opengl
  juce::juce_cryptography
)

function(hum_juce_config target)
  # PUBLIC: host and packs must compile JUCE identically (docs/dev/build.md,
  # "The SDK boundary and the shared JUCE config").
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Without curl JUCE's Linux network path has no TLS: every https call dies.
    find_path(HUM_CURL_INCLUDE_DIR curl/curl.h)
    if(HUM_CURL_INCLUDE_DIR)
      target_compile_definitions(${target} PUBLIC
        JUCE_USE_CURL=1 JUCE_LOAD_CURL_SYMBOLS_LAZILY=1)
      target_include_directories(${target} PUBLIC "${HUM_CURL_INCLUDE_DIR}")
    else()
      message(WARNING "curl/curl.h not found - the app will build but cannot "
                      "reach the update server over https "
                      "(install libcurl4-openssl-dev and re-configure)")
      target_compile_definitions(${target} PUBLIC JUCE_USE_CURL=0)
    endif()
  else()
    target_compile_definitions(${target} PUBLIC JUCE_USE_CURL=0)
  endif()
  target_compile_definitions(${target} PUBLIC
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
    target_compile_definitions(${target} PUBLIC JUCE_PLUGINHOST_AU=1)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_path(HUM_JACK_INCLUDE_DIR jack/jack.h)
    if(HUM_JACK_INCLUDE_DIR)
      target_compile_definitions(${target} PUBLIC JUCE_JACK=1)
    else()
      message(STATUS "jack/jack.h not found - no JACK backend "
                     "(install libjack-jackd2-dev and re-configure to get one)")
    endif()
  endif()
  if(WIN32)
    # Vendored GPLv3 SDK (packaging/README.md, ASIO); OFF removes the component.
    option(HUM_ENABLE_ASIO "Build the ASIO backend (vendored GPLv3 SDK)" ON)
    if(HUM_ENABLE_ASIO)
      if(NOT HUM_ASIO_SDK_DIR AND DEFINED ENV{HUM_ASIO_SDK_DIR})
        set(HUM_ASIO_SDK_DIR "$ENV{HUM_ASIO_SDK_DIR}" CACHE PATH "Steinberg ASIO SDK root")
      endif()
      find_path(HUM_ASIO_INCLUDE_DIR iasiodrv.h
        HINTS "${HUM_ASIO_SDK_DIR}" "${HUM_ASIO_SDK_DIR}/common"
              "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio_sdk" NO_DEFAULT_PATH)
      if(HUM_ASIO_INCLUDE_DIR)
        target_compile_definitions(${target} PUBLIC JUCE_ASIO=1)
        target_include_directories(${target} PUBLIC "${HUM_ASIO_INCLUDE_DIR}")
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
endfunction()

function(hum_add_juce_archive target)
  add_library(${target} STATIC)
  target_link_libraries(${target} PRIVATE ${HUM_JUCE_MODULES})
  target_compile_features(${target} PUBLIC cxx_std_17)
  hum_juce_config(${target})
  target_compile_definitions(${target} INTERFACE
    $<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>)
  target_include_directories(${target} SYSTEM INTERFACE
    $<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>)
endfunction()

hum_add_juce_archive(hum_juce)
# Same test as cmake/Tests.cmake: the source distribution has no tests/.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.cpp)
  hum_add_juce_archive(hum_juce_modal)
  target_compile_definitions(hum_juce_modal PUBLIC JUCE_MODAL_LOOPS_PERMITTED=1)
endif()
