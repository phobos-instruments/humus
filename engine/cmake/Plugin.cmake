option(HUM_PLUGIN_EXPORT "Build the Humus plugin (VST3; +AU on macOS)" ON)

if(HUM_PLUGIN_EXPORT)
  if(APPLE)
    set(HUM_HUMUS_FORMATS VST3 AU)
  else()
    set(HUM_HUMUS_FORMATS VST3)
  endif()
  juce_add_plugin(Humus
    PRODUCT_NAME "Humus"
    COMPANY_NAME "Phobos Instruments"
    COMPANY_WEBSITE "https://humus.phobos-instruments.com"
    BUNDLE_ID "com.phobos-instruments.humus-plugin"
    PLUGIN_MANUFACTURER_CODE Phob
    PLUGIN_CODE Humu
    FORMATS ${HUM_HUMUS_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_AUTO_MANIFEST ${HUM_VST3_MANIFEST})
  target_sources(Humus PRIVATE src/plugin/HumusProcessor.cpp)
  target_include_directories(Humus PRIVATE src ${HUM_GENERATED_DIR})
  add_dependencies(Humus hum_build_id)
  target_link_libraries(Humus PRIVATE hum_core juce::juce_audio_utils)
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
    target_sources(hum_tests PRIVATE src/plugin/HumusProcessor.cpp)
    add_dependencies(hum_tests Humus_VST3)
  endif()
endif()
