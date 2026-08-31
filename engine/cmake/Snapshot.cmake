# The headless GUI render behind the check batteries; built only where the
# fixtures it shares with the tests exist (docs/dev/build.md).
if(HUM_GUI AND HUM_TESTS)
  juce_add_console_app(hum_snapshot)
  target_sources(hum_snapshot PRIVATE
    src/gui/Snapshot.cpp
    src/gui/snapshot/BoardsChrome.cpp
    src/gui/snapshot/BoardsPanes.cpp
    src/gui/snapshot/BoardsScenes.cpp
    src/gui/snapshot/ChecksAutomation.cpp
    src/gui/snapshot/ChecksBox.cpp
    src/gui/snapshot/ChecksEdit.cpp
    src/gui/snapshot/ChecksGraph.cpp
    src/gui/snapshot/ChecksHelp.cpp
    src/gui/snapshot/ChecksLayout.cpp
    src/gui/snapshot/ChecksClip.cpp
    src/gui/snapshot/ChecksHelix.cpp
    src/gui/snapshot/ChecksRoll.cpp
    src/gui/snapshot/ChecksSelect.cpp
    src/gui/snapshot/ChecksSession.cpp
    src/gui/snapshot/ChecksShader.cpp
    src/gui/snapshot/ChecksUnits.cpp
    src/gui/snapshot/ExportOrganisms.cpp
    src/gui/snapshot/HostDiagnostics.cpp
    ${HUM_GUI_SOURCES}
  )
  # Pack headers for the doc exporter; tests/ for the shared fixtures.
  target_include_directories(hum_snapshot PRIVATE src ${HUM_GENERATED_DIR}
                             ../packs/humus/organisms tests)
  add_dependencies(hum_snapshot hum_build_id)
  target_link_libraries(hum_snapshot PRIVATE
    hum_core
    hum_assets
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_devices
    juce::juce_audio_utils
    juce::juce_video        # camera names for "video-inputs" combos
    juce::juce_cryptography # links only - the boards never download
    juce::juce_opengl       # links only - headless render never opens a VisualWindow
  )
  if(APPLE)
    target_link_libraries(hum_snapshot PRIVATE
      "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")
  endif()
  if(HUM_DYN_PACKS)
    # Add-ons arrive as the .humpack this build produced, installed through
    # the ordinary pack path - never linked (docs/dev/build.md).
    target_compile_definitions(hum_snapshot PRIVATE
      HUM_HUMPACKS_DIR="${CMAKE_BINARY_DIR}/humpacks")
    foreach(addon ${HUM_ADDON_PACKS})
      add_dependencies(hum_snapshot humpack_${addon})
    endforeach()
  endif()

  # X11 composite mirror: plugin UIs rendered through our windows.
  if(UNIX AND NOT APPLE)
    find_package(X11 COMPONENTS Xcomposite Xext)
    if(X11_FOUND AND X11_Xcomposite_FOUND)
      foreach(tgt hum_gui hum_snapshot)
        target_link_libraries(${tgt} PRIVATE X11::X11 X11::Xcomposite X11::Xext)
        target_compile_definitions(${tgt} PRIVATE HUM_X11_MIRROR=1)
      endforeach()
    endif()
  endif()

  target_compile_definitions(hum_snapshot PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

  # Stage assets beside the binary so checks exercise the shipped layout.
  add_custom_command(TARGET hum_snapshot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:hum_snapshot>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../assets" "$<TARGET_FILE_DIR:hum_snapshot>/assets"
    COMMAND ${CMAKE_COMMAND} -DDIR=$<TARGET_FILE_DIR:hum_snapshot>/assets
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_junk.cmake"
    COMMENT "Staging assets beside hum_snapshot")
endif()
