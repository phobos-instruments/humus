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
  tests/organisms/OrganismTests.cpp
  tests/organisms/OrganismControlTests.cpp
  tests/organisms/OrganismLiveTests.cpp
  tests/organisms/OrganismDynamicsTests.cpp
  tests/organisms/OrganismRoutingTests.cpp
  tests/organisms/OrganismSourceTests.cpp
  tests/organisms/OrganismFxTests.cpp
  tests/graph/PreparedTests.cpp
  tests/params/ParamRefTests.cpp
  tests/io/RobustInputTests.cpp
  tests/packs/PackSafetyTests.cpp
  tests/packs/AbiFreezeTests.cpp
  tests/dsp/SmoothedGainTests.cpp
  tests/io/RecentFilesTests.cpp
  tests/packs/CategoryTests.cpp
  tests/packs/LayoutProviderTests.cpp
  tests/graph/GraphTests.cpp
  tests/graph/GraphRouteTests.cpp
  tests/graph/BypassTests.cpp
  tests/io/LoaderTests.cpp
  tests/io/LoaderClipTests.cpp
  tests/io/LoaderLegacyTests.cpp
  tests/io/LoaderLaneTests.cpp
  tests/io/Sf2Tests.cpp
  tests/gui/GuiEnginePathTests.cpp
  tests/app/UserPathsTests.cpp
  tests/timeline/AudioTrackTests.cpp
  tests/organisms/ConsoleTests.cpp
  tests/dsp/OversamplingTests.cpp
  tests/organisms/CrossoverTests.cpp
  tests/organisms/DynamicsTests.cpp
  tests/organisms/HelixTests.cpp
  tests/organisms/HelixSessionTests.cpp
  tests/timeline/AutomationTests.cpp
  tests/timeline/ClipOpsTests.cpp
  tests/timeline/AudioClipTests.cpp
  tests/timeline/RecordSessionTests.cpp
  tests/gui/TracksLayoutTests.cpp
  tests/gui/WaveformCacheTests.cpp
  tests/organisms/DeckTests.cpp
  tests/organisms/DeckAnalysisTests.cpp
  tests/organisms/SamplerTests.cpp
  tests/organisms/SamplerBankTests.cpp
  tests/organisms/GraftTests.cpp
  tests/organisms/GraftOrganismTests.cpp
  tests/organisms/LeafcutterTests.cpp
  tests/organisms/LeafcutterFileTests.cpp
  tests/organisms/SoundSpaceTests.cpp
  tests/dsp/DspCoreTests.cpp
  tests/net/ControlShapeTests.cpp
  tests/dsp/FormulaTests.cpp
  tests/io/NumberFormatTests.cpp
  tests/organisms/PrimitivesTests.cpp
  tests/organisms/MathTests.cpp
  tests/organisms/MorseTests.cpp
  tests/dsp/DiodeLadderTests.cpp
  tests/organisms/DropsTests.cpp
  tests/timeline/TapTempoTests.cpp
  tests/organisms/RhythmTests.cpp
  tests/organisms/RhythmFxTests.cpp
  tests/organisms/MicrodotTests.cpp
  tests/organisms/SynthTests.cpp
  tests/organisms/SequencerTests.cpp
  tests/organisms/SideChainTests.cpp
  tests/organisms/SpectralTests.cpp
  tests/organisms/PitchTests.cpp
  tests/tuning/TuningTests.cpp
  tests/tuning/ScaleTests.cpp
  tests/tuning/ScaleLibraryTests.cpp
  tests/timeline/SwingTests.cpp
  tests/timeline/GrooveLaneTests.cpp
  tests/gui/HexColourTests.cpp
  tests/gui/ThemeSchemaTests.cpp
  tests/tuning/MtsTuningTests.cpp
  tests/midi/BendRetunerTests.cpp
  tests/plugins/PianoteqLiveTests.cpp
  tests/organisms/RhizomeTests.cpp
  tests/tuning/TuningNodeTests.cpp
  tests/tuning/TuningScaleTests.cpp
  tests/organisms/StereoToolTests.cpp
  tests/midi/NoteScheduleTests.cpp
  tests/organisms/PhonoTests.cpp
  tests/organisms/WaveTests.cpp
  tests/organisms/WaveVoiceTests.cpp
  tests/organisms/PrismTests.cpp
  tests/midi/ChordTests.cpp
  tests/organisms/DnaTests.cpp
  tests/gui/RollPlotTests.cpp
  tests/assistant/GenieTests.cpp
  tests/params/RandomizeTests.cpp
  tests/gui/LayoutSpecTests.cpp
  tests/gui/LayoutConditionTests.cpp
  tests/packs/RolesTests.cpp
  tests/gui/BrickBindingTests.cpp
  tests/gui/ToolbarLadderTests.cpp
  tests/graph/PodModelTests.cpp
  tests/graph/PodMidiTests.cpp
  tests/packs/PackUpdatesTests.cpp
  tests/params/ParamHistoryTests.cpp
  tests/io/PresetLibraryTests.cpp
  src/gui/editor/LayoutLoader.cpp
  src/gui/editor/LayoutCondition.cpp
  src/gui/settings/OscSerial.cpp
  tests/video/CameraInTests.cpp
  tests/assistant/AssistantTests.cpp
  tests/assistant/AutomixPlanTests.cpp
  tests/dsp/SpectrumBandsTests.cpp
  tests/assistant/RecipeSynthTests.cpp
  tests/assistant/OllamaWireTests.cpp
  tests/packs/PackSdkTests.cpp
  tests/plugins/PluginHostTests.cpp
  tests/plugins/PluginInstanceTests.cpp
  tests/plugins/PluginPatchTests.cpp
  tests/plugins/PluginDogfoodTests.cpp
  tests/plugins/PluginListStoreTests.cpp
  tests/gui/LooperFlowTests.cpp
  tests/params/MetapadTests.cpp
  tests/midi/MidiBusTests.cpp
  tests/params/ParamGroupingTests.cpp
  tests/app/PickerModelTests.cpp
  tests/app/WizardFlowTests.cpp
  tests/app/GuideFlowTests.cpp
  tests/video/SkeletonTests.cpp
  tests/organisms/GritTests.cpp
  tests/organisms/LoudnessProbeTests.cpp
  tests/video/HandsTests.cpp
  tests/gui/SigilTests.cpp
  tests/organisms/SiltTests.cpp
  tests/midi/MelodyTraceTests.cpp
  tests/midi/MidiClipImportTests.cpp
  tests/organisms/SliderTests.cpp
  tests/organisms/ButtonTests.cpp
  tests/midi/MidiChordTests.cpp
  tests/packs/CautionTests.cpp
  tests/video/VideoTrackTests.cpp
  tests/video/VideoTakeClockTests.cpp
  tests/io/MediaProbeTests.cpp
  tests/organisms/Coupling303Tests.cpp
  tests/organisms/AcidTests.cpp
  tests/organisms/PitchBendTests.cpp
  tests/organisms/RiffTests.cpp
  tests/io/RiffImportTests.cpp
  tests/organisms/SequenceTests.cpp
  tests/organisms/SirenTests.cpp
  tests/video/DxtDecodeTests.cpp
  tests/video/PadBarTests.cpp
  tests/organisms/CrossfaderCutTests.cpp
  tests/video/VideoPadTests.cpp
  tests/organisms/FollowerSensorTests.cpp
  tests/graph/GraphHandoverTests.cpp
  tests/gui/UiTickerTests.cpp
  tests/organisms/GateTests.cpp
  tests/organisms/ClusterTests.cpp
  tests/organisms/HarmonizerTests.cpp
  tests/organisms/PaulstretchTests.cpp
  tests/organisms/PinkTromboneTests.cpp
  tests/midi/MidiTrackTests.cpp
  tests/midi/MidiGraphTests.cpp
  tests/midi/MidiLaneTests.cpp
  tests/midi/MidiRollTests.cpp
  tests/midi/MidiCodecTests.cpp
  tests/midi/MidiMonitorTests.cpp
  tests/midi/MidiSyncTests.cpp
  tests/net/LinkChaseTests.cpp
  tests/timeline/MeterTests.cpp
  tests/io/MediaRefsTests.cpp
  tests/io/SignalFileTests.cpp
  tests/video/HalogenVideoTests.cpp
  tests/io/ExamplesTests.cpp
  tests/net/LinkVendorTests.cpp
  tests/graph/RtWordTests.cpp
  tests/graph/GraphLayoutTests.cpp
  tests/graph/AutoWireTests.cpp
  tests/net/OscTests.cpp
  tests/graph/ModControlTests.cpp
  tests/video/VisualUniformsTests.cpp
  tests/video/IsfParseTests.cpp
  tests/video/SsfShimTests.cpp
  tests/timeline/MixRecorderTests.cpp
  tests/graph/HumusMacroTests.cpp
  tests/graph/HumusPodTests.cpp
  tests/net/SerialTests.cpp
  tests/net/SerialPortTests.cpp
  tests/net/BoardTests.cpp
  tests/net/OscSerialTests.cpp
  tests/organisms/PhTests.cpp
  tests/organisms/PhBankTests.cpp
  tests/organisms/PhChipTests.cpp
  tests/organisms/LfoScopeTests.cpp
  tests/graph/LatencyTests.cpp
  tests/io/PerfBoxTests.cpp
  tests/io/AutosaveTests.cpp
  tests/app/UiWatchdogTests.cpp
  tests/plugins/BridgeRingTests.cpp
  tests/plugins/BridgeTests.cpp
  tests/net/LicenseCheckTests.cpp
  tests/plugins/HelperLaunchTests.cpp
  tests/video/VideoHwAccelTests.cpp
  tests/net/HubProtocolTests.cpp
  tests/net/TelemetrySpoolTests.cpp
  tests/gui/HelpTests.cpp
  tests/organisms/ConsoleTests.cpp
  tests/dsp/OversamplingTests.cpp
  tests/organisms/MixerTests.cpp
  tests/io/DocumentSetTests.cpp)
target_include_directories(hum_tests PRIVATE tests)
target_link_libraries(hum_tests PRIVATE hum_core hum_assets)
foreach(addon ${HUM_ADDON_PACKS})
  target_link_libraries(hum_tests PRIVATE hum_pack_${addon})
  target_compile_definitions(hum_tests PRIVATE
    HUM_STATIC_ADDON_PACKS=1
    HUM_ADDON_ID="${addon}"
    HUM_ADDON_REGISTER=hum_register_pack_${addon})
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
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_AUTO_MANIFEST ${HUM_VST3_MANIFEST})
  target_sources(hum_testgain PRIVATE tests/fixtures/testgain/TestGain.cpp)
  target_link_libraries(hum_testgain PRIVATE juce::juce_audio_utils)
  target_compile_definitions(hum_testgain PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_compile_definitions(hum_tests PRIVATE
    HUM_TESTGAIN_VST3_DIR="$<TARGET_FILE_DIR:hum_testgain_VST3>/../../..")
  add_dependencies(hum_tests hum_testgain_VST3)
endif()
