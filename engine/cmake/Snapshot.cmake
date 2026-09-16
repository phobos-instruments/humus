# The headless GUI render behind the check batteries (docs/dev/build.md).
if(HUM_GUI AND HUM_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/snapshot/Snapshot.cpp)
  juce_add_console_app(hum_snapshot)
  target_sources(hum_snapshot PRIVATE
    src/gui/snapshot/Snapshot.cpp
    src/gui/snapshot/BoardsChrome.cpp
    src/gui/snapshot/BoardsPanes.cpp
    src/gui/snapshot/BoardsScenes.cpp
    src/gui/snapshot/ChecksAutomation.cpp
    src/gui/snapshot/ChecksLatch.cpp
    src/gui/snapshot/ChecksWaveMorph.cpp
    src/gui/snapshot/ChecksBounce.cpp
    src/gui/snapshot/ChecksBounceAudio.cpp
    src/gui/snapshot/ChecksBounceExport.cpp
    src/gui/snapshot/ChecksBounceJob.cpp
    src/gui/snapshot/ChecksBounceVideo.cpp
    src/gui/snapshot/MovieProbe.cpp
    src/gui/snapshot/ChecksBox.cpp
    src/gui/snapshot/ChecksMeters.cpp
    src/gui/snapshot/ChecksPresetStack.cpp
    src/gui/snapshot/ChecksSampler.cpp
    src/gui/snapshot/ChecksRack.cpp
    src/gui/snapshot/ChecksBoxBench.cpp
    src/gui/snapshot/ChecksEdit.cpp
    src/gui/snapshot/ChecksSaves.cpp
    src/gui/snapshot/ChecksParamEdit.cpp
    src/gui/snapshot/ChecksMap.cpp
    src/gui/snapshot/ChecksGraph.cpp
    src/gui/snapshot/ChecksPods.cpp
    src/gui/snapshot/ChecksPodText.cpp
    src/gui/snapshot/ChecksVideoTrack.cpp
    src/gui/snapshot/ChecksVideoClips.cpp
    src/gui/snapshot/ChecksVideoRecord.cpp
    src/gui/snapshot/ChecksVideoSources.cpp
    src/gui/snapshot/ChecksVideoRender.cpp
    src/gui/snapshot/ChecksHap.cpp
    src/gui/snapshot/ChecksGraphBench.cpp
    src/gui/snapshot/ChecksText.cpp
    src/gui/snapshot/ChecksRebuild.cpp
    src/gui/snapshot/ChecksHelp.cpp
    src/gui/snapshot/ChecksLayout.cpp
    src/gui/snapshot/ChecksPaneLayout.cpp
    src/gui/snapshot/ChecksTrackRoll.cpp
    src/gui/snapshot/ChecksTracksBench.cpp
    src/gui/snapshot/ChecksTrackEdit.cpp
    src/gui/snapshot/ChecksTrackAdd.cpp
    src/gui/snapshot/ChecksTrackKeys.cpp
    src/gui/snapshot/ChecksDeviceNotice.cpp
    src/gui/snapshot/ChecksTrackLanes.cpp
    src/gui/snapshot/ChecksSpinner.cpp
    src/gui/snapshot/ChecksRecorder.cpp
    src/gui/snapshot/ChecksLanePoints.cpp
    src/gui/snapshot/ChecksMidiPlayer.cpp
    src/gui/snapshot/ChecksRiffImport.cpp
    src/gui/snapshot/ChecksGroove.cpp
    src/gui/snapshot/ChecksTheme.cpp
    src/gui/snapshot/ChecksGenie.cpp
    src/gui/snapshot/ChecksCanvas.cpp
    src/gui/snapshot/ChecksSearch.cpp
    src/gui/snapshot/ChecksParamControl.cpp
    src/gui/snapshot/ChecksGrit.cpp
    src/gui/snapshot/ChecksDeclaredRoll.cpp
    src/gui/snapshot/ChecksBrickBinding.cpp
    src/gui/snapshot/ChecksClip.cpp
    src/gui/snapshot/ChecksHelix.cpp
    src/gui/snapshot/ChecksRoll.cpp
    src/gui/snapshot/ChecksAssets.cpp
    src/gui/snapshot/ChecksSockets.cpp
    src/gui/snapshot/ChecksRollOrganism.cpp
    src/gui/snapshot/ChecksSelect.cpp
    src/gui/snapshot/ChecksSession.cpp
    src/gui/snapshot/ChecksMedia.cpp
    src/gui/snapshot/ChecksMidiInput.cpp
    src/gui/snapshot/ChecksTimeSig.cpp
    src/gui/snapshot/ChecksDevice.cpp
    src/gui/snapshot/ChecksShader.cpp
    src/gui/snapshot/ChecksUnits.cpp
    src/gui/snapshot/ChecksWelcome.cpp
    src/gui/snapshot/ExportOrganisms.cpp
    src/gui/snapshot/ExportPose.cpp
    src/gui/snapshot/DiagProbes.cpp
    src/gui/snapshot/DiagPlugins.cpp
    src/gui/snapshot/DiagEmbed.cpp
    ${HUM_GUI_SOURCES}
  )
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
    juce::juce_video
    juce::juce_cryptography
    juce::juce_opengl
  )
  hum_link_video(hum_snapshot)
  if(HUM_DYN_PACKS)
    # Add-ons arrive as this build's .humpack through the ordinary pack path.
    target_compile_definitions(hum_snapshot PRIVATE
      HUM_HUMPACKS_DIR="${CMAKE_BINARY_DIR}/humpacks")
    foreach(addon ${HUM_ADDON_PACKS})
      add_dependencies(hum_snapshot humpack_${addon})
    endforeach()
  endif()

  if(UNIX AND NOT APPLE)
    find_package(X11 COMPONENTS Xcomposite Xext)
    if(X11_FOUND AND X11_Xcomposite_FOUND)
      foreach(tgt hum_gui hum_snapshot)
        target_link_libraries(${tgt} PRIVATE X11::X11 X11::Xcomposite X11::Xext)
        target_compile_definitions(${tgt} PRIVATE HUM_X11_MIRROR=1)
      endforeach()
    endif()
  endif()

  # runDispatchLoopUntil() for the organism exporter: runDispatchLoop() cannot
  # be stopped and restarted in one process (the quit flag never clears).
  target_compile_definitions(hum_snapshot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_MODAL_LOOPS_PERMITTED=1)

  add_custom_command(TARGET hum_snapshot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:hum_snapshot>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../assets" "$<TARGET_FILE_DIR:hum_snapshot>/assets"
    COMMAND ${CMAKE_COMMAND} -DDIR=$<TARGET_FILE_DIR:hum_snapshot>/assets
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_junk.cmake"
    COMMENT "Staging assets beside hum_snapshot")
endif()
