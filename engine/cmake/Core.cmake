add_library(hum_core STATIC
  src/core/AudioGraph.cpp
  src/core/LicenseCheck.cpp
  src/core/Registry.cpp
  src/core/BuiltinPacks.cpp
  src/core/ParamSchema.cpp
  src/core/ParamUnit.cpp
  src/core/Categories.cpp
  src/core/PackManifest.cpp
  src/core/PackRegistry.cpp
  src/core/PackLoader.cpp
  src/core/PackUpdates.cpp
  src/core/PluginHost.cpp
  src/core/PluginListStore.cpp
  src/core/ScaleLibrary.cpp
  src/core/MtsTuning.cpp
  src/core/TuningProbe.cpp
  src/core/TuningProbeStore.cpp
  src/core/HostedPlugin.cpp
  src/core/PluginScanner.cpp
  src/core/BridgeRing.cpp
  src/core/BridgeClient.cpp
  src/core/BridgeWorker.cpp
  src/core/BridgedPlugin.cpp
  src/core/BeatDetector.cpp
  src/core/KeyDetector.cpp
  src/core/PresetGenie.cpp
  src/core/AssistantProtocol.cpp
  src/core/OllamaWire.cpp
  src/core/RecipeSynth.cpp
  src/io/PatchWriter.cpp
  src/io/PatchWriterElements.cpp
  src/io/PatchDocument.cpp
  src/io/PatchDocumentPattern.cpp
  src/io/PatchLoader.cpp
  src/io/WavWriter.cpp
  src/io/AutosaveStore.cpp
)
target_include_directories(hum_core PUBLIC src)
# Dev fallback; shipped builds resolve <exeDir>/packs first.
target_compile_definitions(hum_core PUBLIC
  HUM_PACKS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../packs")
target_link_libraries(hum_core PUBLIC
  monocypher
  ableton_link
  hum_sdk
  hum_pack_core
  hum_pack_humus
  hum_pack_av
  juce::juce_core
  juce::juce_audio_basics
  juce::juce_audio_formats
  juce::juce_audio_processors
  juce::juce_dsp
  juce::juce_osc
)
if(APPLE)
  # AU hosting references AUGenericView; plain executables need it spelled out.
  target_link_libraries(hum_core PUBLIC "-framework CoreAudioKit")
  target_link_libraries(hum_core PUBLIC "-framework GameController")
endif()
