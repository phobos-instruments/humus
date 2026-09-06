# The source distribution arrives without tests/ and builds none.
set(HUM_TESTS OFF)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.cpp)
  set(HUM_TESTS ON)
endif()

if(HUM_TESTS)
add_executable(hum_tests tests/run_tests.cpp)
file(GLOB HUM_PACK_TESTS CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/../packs/*/tests/*.cpp)
target_sources(hum_tests PRIVATE ${HUM_PACK_TESTS})
target_sources(hum_tests PRIVATE
  tests/OrganismTests.cpp
  tests/RecentFilesTests.cpp
  tests/CategoryTests.cpp
  tests/LayoutProviderTests.cpp
  tests/GraphTests.cpp
  tests/BypassTests.cpp
  tests/DelayTests.cpp
  tests/LoaderTests.cpp
  tests/Sf2Tests.cpp
  tests/GuiEnginePathTests.cpp
  tests/AudioTrackTests.cpp
  tests/CrossoverTests.cpp
  tests/DynamicsTests.cpp
  tests/HelixTests.cpp
  tests/AutomationTests.cpp
  tests/ClipOpsTests.cpp
  tests/RecordSessionTests.cpp
  tests/TracksLayoutTests.cpp
  tests/WaveformCacheTests.cpp
  tests/DeckTests.cpp
  tests/SamplerTests.cpp
  tests/LeafcutterTests.cpp
  tests/SoundSpaceTests.cpp
  tests/DspCoreTests.cpp
  tests/ControlShapeTests.cpp
  tests/FormulaTests.cpp
  tests/NumberFormatTests.cpp
  tests/PrimitivesTests.cpp
  tests/MorseTests.cpp
  tests/DiodeLadderTests.cpp
  tests/DropsTests.cpp
  tests/TapTempoTests.cpp
  tests/TribeTests.cpp
  tests/SynthTests.cpp
  tests/SideChainTests.cpp
  tests/SpectralTests.cpp
  tests/PitchTests.cpp
  tests/TuningTests.cpp
  tests/ScaleTests.cpp
  tests/ScaleLibraryTests.cpp
  tests/SwingTests.cpp
  tests/MtsTuningTests.cpp
  tests/BendRetunerTests.cpp
  tests/PianoteqLiveTests.cpp
  tests/RhizomeTests.cpp
  tests/TuningNodeTests.cpp
  tests/StereoToolTests.cpp
  tests/NoteScheduleTests.cpp
  tests/PhonoTests.cpp
  tests/WaveTests.cpp
  tests/PrismTests.cpp
  tests/ChordTests.cpp
  tests/DnaTests.cpp
  tests/RollPlotTests.cpp
  tests/GenieTests.cpp
  tests/RandomizeTests.cpp
  tests/LayoutSpecTests.cpp
  tests/ToolbarLadderTests.cpp
  tests/PodModelTests.cpp
  tests/PodMidiTests.cpp
  tests/PackUpdatesTests.cpp
  tests/ParamHistoryTests.cpp
  tests/PresetLibraryTests.cpp
  src/gui/LayoutLoader.cpp
  tests/CameraInTests.cpp
  tests/AssistantTests.cpp
  tests/AutomixPlanTests.cpp
  tests/SpectrumBandsTests.cpp
  tests/RecipeSynthTests.cpp
  tests/OllamaWireTests.cpp
  tests/PackSdkTests.cpp
  tests/PluginHostTests.cpp
  tests/PluginListStoreTests.cpp
  tests/LooperFlowTests.cpp
  tests/MetapadTests.cpp
  tests/MidiBusTests.cpp
  tests/ParamGroupingTests.cpp
  tests/PickerModelTests.cpp
  tests/WizardFlowTests.cpp
  tests/GuideFlowTests.cpp
  tests/SkeletonTests.cpp
  tests/GritTests.cpp
  tests/LoudnessProbeTests.cpp
  tests/HandsTests.cpp
  tests/SigilTests.cpp
  tests/SiltTests.cpp
  tests/MelodyTraceTests.cpp
  tests/SliderTests.cpp
  tests/ButtonTests.cpp
  tests/MidiChordTests.cpp
  tests/CautionTests.cpp
  tests/VideoTrackTests.cpp
  tests/VideoTakeClockTests.cpp
  tests/MediaProbeTests.cpp
  tests/Coupling303Tests.cpp
  tests/AcidTests.cpp
  tests/RiffTests.cpp
  tests/SequenceTests.cpp
  tests/DxtDecodeTests.cpp
  tests/PadBarTests.cpp
  tests/CrossfaderCutTests.cpp
  tests/VideoPadTests.cpp
  tests/FollowerSensorTests.cpp
  tests/GraphHandoverTests.cpp
  tests/UiTickerTests.cpp
  tests/GateTests.cpp
  tests/ClusterTests.cpp
  tests/HarmonizerTests.cpp
  tests/PaulstretchTests.cpp
  tests/PinkTromboneTests.cpp
  tests/MidiTrackTests.cpp
  tests/MidiGraphTests.cpp
  tests/MidiMonitorTests.cpp
  tests/MidiSyncTests.cpp
  tests/LinkChaseTests.cpp
  tests/LinkVendorTests.cpp
  tests/RtWordTests.cpp
  tests/GraphLayoutTests.cpp
  tests/AutoWireTests.cpp
  tests/OscTests.cpp
  tests/ModControlTests.cpp
  tests/VisualUniformsTests.cpp
  tests/IsfParseTests.cpp
  tests/SsfShimTests.cpp
  tests/MixRecorderTests.cpp
  tests/HumusMacroTests.cpp
  tests/HumusPodTests.cpp
  tests/SerialTests.cpp
  tests/PhTests.cpp
  tests/LfoScopeTests.cpp
  tests/LatencyTests.cpp
  tests/PerfBoxTests.cpp
  tests/AutosaveTests.cpp
  tests/UiWatchdogTests.cpp
  tests/BridgeRingTests.cpp
  tests/BridgeTests.cpp
  tests/LicenseCheckTests.cpp
  tests/HubProtocolTests.cpp
  tests/TelemetrySpoolTests.cpp
  tests/HelpTests.cpp
  tests/ConsoleTests.cpp
  tests/MixerTests.cpp
  tests/DocumentSetTests.cpp)
target_link_libraries(hum_tests PRIVATE hum_core hum_assets)
foreach(addon ${HUM_ADDON_PACKS})
  target_link_libraries(hum_tests PRIVATE hum_pack_${addon})
  target_compile_definitions(hum_tests PRIVATE HUM_STATIC_ADDON_PACKS=1)
endforeach()
target_compile_definitions(hum_tests PRIVATE
  HUM_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/..")
if(HUM_DYN_PACKS)
  target_compile_definitions(hum_tests PRIVATE
    HUM_PACKS_DYN_DIR="${CMAKE_BINARY_DIR}/packs_dyn"
    HUM_HUMPACKS_DIR="${CMAKE_BINARY_DIR}/humpacks")
  add_dependencies(hum_tests
    hum_pack_core_dyn hum_pack_humus_dyn hum_pack_av_dyn humpack_sample)
  foreach(addon ${HUM_ADDON_PACKS})
    add_dependencies(hum_tests hum_pack_${addon}_dyn)
  endforeach()
endif()
endif()   # tests present

option(HUM_HOSTING_TESTS "Build the plugin-hosting test fixture + tests" ON)
if(HUM_HOSTING_TESTS AND HUM_TESTS)
  juce_add_plugin(hum_testgain
    PRODUCT_NAME "TestGain"
    COMPANY_NAME "Humus"
    PLUGIN_MANUFACTURER_CODE Humt
    PLUGIN_CODE Tgn1
    FORMATS VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  target_sources(hum_testgain PRIVATE tests/fixtures/testgain/TestGain.cpp)
  target_link_libraries(hum_testgain PRIVATE juce::juce_audio_utils)
  target_compile_definitions(hum_testgain PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_compile_definitions(hum_tests PRIVATE
    HUM_TESTGAIN_VST3_DIR="$<TARGET_FILE_DIR:hum_testgain_VST3>/../../..")
  add_dependencies(hum_tests hum_testgain_VST3)
endif()
