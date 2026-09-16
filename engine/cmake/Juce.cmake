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

# The tag is what the fetch asks for; the SHA is what the patches were written
# against. A moved tag fails here instead of in a patch anchor.
set(HUM_JUCE_SHA "51d11a2be6d5c97ccf12b4e5e827006e19f0555a")
find_package(Git QUIET)
if(GIT_EXECUTABLE AND EXISTS "${juce_SOURCE_DIR}/.git")
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
                  WORKING_DIRECTORY "${juce_SOURCE_DIR}"
                  OUTPUT_VARIABLE HUM_JUCE_HEAD OUTPUT_STRIP_TRAILING_WHITESPACE
                  RESULT_VARIABLE HUM_JUCE_HEAD_RESULT)
  if(HUM_JUCE_HEAD_RESULT EQUAL 0 AND NOT HUM_JUCE_HEAD STREQUAL HUM_JUCE_SHA)
    message(FATAL_ERROR "JUCE at ${juce_SOURCE_DIR} is ${HUM_JUCE_HEAD}, expected ${HUM_JUCE_SHA} (tag 8.0.4 moved?)")
  endif()
endif()

# Quieted per file, not per target (docs/dev/build.md, "The multichar quieting").
if(NOT MSVC)
  set_source_files_properties(
    "${juce_SOURCE_DIR}/modules/juce_audio_processors/juce_audio_processors.cpp"
    "${juce_SOURCE_DIR}/modules/juce_audio_processors/juce_audio_processors.mm"
    "${juce_SOURCE_DIR}/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp"
    PROPERTIES COMPILE_OPTIONS "-Wno-multichar")
endif()
