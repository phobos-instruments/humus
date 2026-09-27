# The JUCE-free half of the host: JSON, pack and preset loading, the param
# schema, the faceplate controller, the brick models and the piano roll model.
# It links nothing from JUCE, so a JUCE include anywhere in it fails to
# compile; hum_core links it for the app, hum_face_probe for the purity probe
# (docs/dev/headless-host-and-bricks.md, Part E).
add_library(hum_models STATIC
  src/core/browser/BrowserIndex.cpp
  src/core/browser/BrowserModel.cpp
  src/core/browser/BrowserPlaces.cpp
  src/core/browser/BrowserQuery.cpp
  src/core/browser/FileIndex.cpp
  src/core/browser/FolderScan.cpp
  src/core/browser/FileIndexStore.cpp
  src/core/browser/FileKind.cpp
  src/core/browser/FileTimes.cpp
  src/core/browser/NameFacts.cpp
  src/core/browser/PlaceMemory.cpp
  src/core/browser/ProjectFacts.cpp
  src/core/browser/ThreadPriority.cpp
  src/core/json/JsonParse.cpp
  src/core/json/JsonValue.cpp
  src/core/json/JsonWrite.cpp
  src/core/packs/Categories.cpp
  src/core/packs/PackManifest.cpp
  src/core/packs/PackRegistry.cpp
  src/core/packs/PresetDefs.cpp
  src/core/packs/Registry.cpp
  src/core/packs/Roles.cpp
  src/core/params/ParamSchema.cpp
  src/core/params/UnitText.cpp
  src/core/plugins/HostedPlugins.cpp
  src/core/project/ProjectFolder.cpp
  src/core/xml/XmlElement.cpp
  src/core/xml/XmlParse.cpp
  src/core/xml/XmlWrite.cpp
  src/gui/editor/Faceplate.cpp
  src/gui/editor/FaceplateCombo.cpp
  src/gui/editor/FaceplateControls.cpp
  src/gui/editor/FaceplateInspect.cpp
  src/gui/editor/LayoutCondition.cpp
  src/gui/editor/LayoutLoader.cpp
  src/gui/editor/decks/DeckModels.cpp
  src/gui/editor/files/PictureFieldModel.cpp
  src/gui/editor/files/SliceMapModel.cpp
  src/gui/editor/grids/GainShapeModel.cpp
  src/gui/editor/grids/PatternEditorModel.cpp
  src/gui/editor/grids/StepGridModel.cpp
  src/gui/editor/grids/WaveDrawModel.cpp
  src/gui/editor/inputs/NumberInputs.cpp
  src/gui/editor/inputs/RangeInput.cpp
  src/gui/editor/inputs/StripInputs.cpp
  src/gui/editor/readouts/ScopeReadings.cpp
  src/gui/editor/video/ClipPadsModel.cpp
  src/gui/editor/video/VideoModels.cpp
  src/gui/pianoroll/RollEdits.cpp
  src/gui/pianoroll/RollGestures.cpp
  src/gui/pianoroll/RollTransport.cpp
)
target_include_directories(hum_models PUBLIC
  src
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include)
target_compile_features(hum_models PUBLIC cxx_std_17)
