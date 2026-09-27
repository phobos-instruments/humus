# The EngineHost as a library, linked by hum_gui and hum_snapshot (docs/dev/headless-host-and-bricks.md).
if(HUM_GUI)
  add_library(hum_host STATIC ${HUM_HOST_SOURCES})
  target_include_directories(hum_host PUBLIC src ${HUM_GENERATED_DIR})
  add_dependencies(hum_host hum_build_id)
  target_link_libraries(hum_host PUBLIC
    hum_core
    hum_assets
  )
  hum_link_video(hum_host)
  target_compile_definitions(hum_host PRIVATE JUCE_WEB_BROWSER=0)
  hum_warnings(hum_host)

  # hum_tests drives a real host through tests/host/TestHost.h.
  if(TARGET hum_tests)
    target_sources(hum_tests PRIVATE
      tests/host/HostRenderTests.cpp
      tests/host/CaptureTests.cpp
      tests/host/RollRecordTests.cpp
      tests/host/LaneEditTests.cpp
      tests/host/LatchTests.cpp
      tests/host/TakeTests.cpp
      tests/host/HistoryTests.cpp
      tests/host/MidiTakeTests.cpp
      tests/host/TrackInputTests.cpp
      tests/host/TrackMuteTests.cpp
      tests/host/PanicTests.cpp
      tests/host/UndoTests.cpp
      tests/host/PodTests.cpp
      tests/host/InsertTests.cpp
      tests/host/SaveTests.cpp
      tests/host/DeckPortTests.cpp
      tests/host/DeckTriggerTests.cpp
      tests/host/MarkerTests.cpp
      tests/host/VideoPadAudioTests.cpp
      tests/host/HostTeardownTests.cpp
      tests/host/SocketTests.cpp
      tests/host/ClockTests.cpp
      tests/host/LevelMeterTests.cpp
      tests/host/DevicePathTests.cpp
      tests/host/ParamEditTests.cpp
      tests/host/MapTests.cpp
      tests/host/RangeEndTests.cpp
      tests/host/MidiInputTests.cpp
      tests/host/PresetLayerTests.cpp
      tests/host/DeclaredRollTests.cpp
      tests/host/AssistantLevelTests.cpp
      tests/host/BankTests.cpp
      tests/host/SequenceRowsTests.cpp
      tests/host/GrooveTests.cpp
      tests/host/WaveMorphTests.cpp
      tests/host/HostMidiPlayerTests.cpp
      tests/host/FileSwapTests.cpp
      tests/host/PrintTests.cpp
      tests/host/ConsolidateTests.cpp
      tests/host/TrackLaneTests.cpp
      tests/host/MissingMediaTests.cpp
      tests/host/MissingOrganismTests.cpp
      tests/host/HelixSaveTests.cpp
      tests/host/ProjectSaveTests.cpp
      tests/host/RollScopeHostTests.cpp
      tests/host/MetapadScopeHostTests.cpp
      tests/host/MetapadMaskPanelTests.cpp)
    target_link_libraries(hum_tests PRIVATE hum_host)
    # runDispatchLoopUntil() pumps the loop for tests that wait on async work.
    target_compile_definitions(hum_tests PRIVATE JUCE_WEB_BROWSER=0)
  endif()
endif()
