option(HUM_PLUGIN_EXPORT "Build the Humus plugin (VST3; +AU on macOS)" ON)

# The plug-in runs the app's own EngineHost and shows the app's interface, so it
# needs hum_interface: this file is included after Interface.cmake.
if(HUM_PLUGIN_EXPORT AND TARGET hum_interface)
  if(APPLE)
    set(HUM_HUMUS_FORMATS VST3 AU)
  else()
    set(HUM_HUMUS_FORMATS VST3)
  endif()
  juce_add_plugin(Humus
    PRODUCT_NAME "Humus"
    COMPANY_NAME "Phobos Instruments"
    COMPANY_WEBSITE "https://humus.art"
    BUNDLE_ID "com.phobos-instruments.humus-plugin"
    PLUGIN_MANUFACTURER_CODE Phob
    PLUGIN_CODE Humu
    FORMATS ${HUM_HUMUS_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_AUTO_MANIFEST ${HUM_VST3_MANIFEST})
  target_sources(Humus PRIVATE src/plugin/HumusProcessor.cpp src/plugin/HumusEditor.cpp src/plugin/HumusEntry.cpp
                               src/plugin/PluginSession.cpp src/plugin/PatchPanes.cpp
                               src/plugin/VideoWindows.cpp src/plugin/DesktopSession.cpp)
  target_include_directories(Humus PRIVATE src ${HUM_GENERATED_DIR})
  add_dependencies(Humus hum_build_id)
  target_link_libraries(Humus PRIVATE hum_interface juce::juce_audio_utils)
  # The media (Media.cmake: video, and FFmpeg where the platform needs it)
  # goes into each format's module itself, not the shared-code archive, which
  # GNU ld reads before the hosts that call it. hum_media_none in its place
  # gives a plug-in without video and without FFmpeg.
  foreach(format ${HUM_HUMUS_FORMATS})
    hum_link_video(Humus_${format})
  endforeach()
  # A Linux DAW loads every plug-in into one process, and an exported symbol
  # can bind to another plug-in's copy of the same code (two JUCE builds, two
  # FFmpegs). Only the VST3 entry points leave the module; packs import
  # nothing from it, so hiding the rest costs nothing.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET Humus_VST3)
    set(HUM_PLUGIN_EXPORTS "${CMAKE_CURRENT_BINARY_DIR}/humus-plugin-exports.map")
    file(WRITE "${HUM_PLUGIN_EXPORTS}"
      "{\n  global: GetPluginFactory; ModuleEntry; ModuleExit;\n  local: *;\n};\n")
    target_link_options(Humus_VST3 PRIVATE "-Wl,--version-script=${HUM_PLUGIN_EXPORTS}")
  endif()
  target_compile_definitions(Humus PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  # Each format stages its own packs copy: a host resolves beside ITS binary.
  if(APPLE)
    set(HUM_PLUGIN_TS "--timestamp")
    if(HUM_CODESIGN_IDENTITY STREQUAL "-")
      set(HUM_PLUGIN_TS "--timestamp=none")
    endif()
  endif()
  function(hum_stage_plugin_packs target)
    if(APPLE)
      set(dest "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/packs")
      set(adest "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/assets")
    else()
      set(dest "$<TARGET_FILE_DIR:${target}>/packs")
      set(adest "$<TARGET_FILE_DIR:${target}>/assets")
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -rf "${dest}" "$<TARGET_FILE_DIR:${target}>/packs"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../packs/core" "${dest}/core"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../packs/humus" "${dest}/humus"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av" "${dest}/av"
      COMMAND ${CMAKE_COMMAND} -DDIR=${dest}
              -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_pack_sources.cmake"
      COMMAND ${CMAKE_COMMAND} -DDIR=${dest}
              -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_junk.cmake"
      COMMAND ${CMAKE_COMMAND} -E rm -rf "${adest}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets" "${adest}"
      COMMAND ${CMAKE_COMMAND} -E rm -rf "${adest}/patches"
      COMMAND ${CMAKE_COMMAND} -DDIR=${adest}
              -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_junk.cmake"
      COMMENT "Staging organism packs and assets into ${target}")
    if(APPLE)
      # Staging lands after JUCE's seal: re-sign.
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND codesign --force --options runtime ${HUM_PLUGIN_TS}
                --sign "${HUM_CODESIGN_IDENTITY}" "$<TARGET_BUNDLE_DIR:${target}>"
        COMMENT "Sealing ${target} (identity: ${HUM_CODESIGN_IDENTITY})")
    endif()
  endfunction()
  hum_stage_plugin_packs(Humus_VST3)
  if(APPLE)
    hum_stage_plugin_packs(Humus_AU)
  endif()

  if(HUM_HOSTING_TESTS AND HUM_TESTS)
    target_compile_definitions(hum_tests PRIVATE
      HUM_HUMUS_VST3_DIR="$<TARGET_FILE_DIR:Humus_VST3>/../../.."
      HUM_HAVE_HUMUS_PLUGIN=1)
    target_sources(hum_tests PRIVATE src/plugin/HumusProcessor.cpp src/plugin/HumusEditor.cpp src/plugin/PluginSession.cpp
                                     src/plugin/PatchPanes.cpp src/plugin/VideoWindows.cpp
                                     src/plugin/DesktopSession.cpp
                                     tests/plugins/HostedPluginTests.cpp tests/plugins/HumusPluginDawTests.cpp
                                     tests/plugins/HumusPluginEditTests.cpp tests/plugins/HumusPluginSessionTests.cpp
                                     tests/plugins/HumusPluginMidiTests.cpp
                                     tests/plugins/HumusPluginGuardTests.cpp
                                     tests/plugins/HumusPluginVideoTests.cpp
                                     tests/plugins/HumusPluginWindowTests.cpp
                                     tests/plugins/ClapWindowTests.cpp)
    target_link_libraries(hum_tests PRIVATE hum_interface)
    add_dependencies(hum_tests Humus_VST3)
  endif()
  hum_warnings(Humus)
endif()
