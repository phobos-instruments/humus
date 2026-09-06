include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG        8.0.4
  GIT_SHALLOW    TRUE
  # docs/dev/build.md, "JUCE, and the three patches".
  PATCH_COMMAND  ${CMAKE_COMMAND} -DJUCE_SOURCE_DIR=<SOURCE_DIR>
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchJuceCoreAudio.cmake
         COMMAND ${CMAKE_COMMAND} -DJUCE_SOURCE_DIR=<SOURCE_DIR>
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchJuceDirect2D.cmake
         COMMAND ${CMAKE_COMMAND} -DJUCE_SOURCE_DIR=<SOURCE_DIR>
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchJuceNetworkLinux.cmake
)
FetchContent_MakeAvailable(JUCE)

# Quieted per file, not per target (docs/dev/build.md, "The multichar quieting").
if(NOT MSVC)
  set_source_files_properties(
    "${juce_SOURCE_DIR}/modules/juce_audio_processors/juce_audio_processors.cpp"
    "${juce_SOURCE_DIR}/modules/juce_audio_processors/juce_audio_processors.mm"
    "${juce_SOURCE_DIR}/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp"
    PROPERTIES COMPILE_OPTIONS "-Wno-multichar")
endif()
