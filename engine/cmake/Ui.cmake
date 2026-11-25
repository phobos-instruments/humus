# The interface the app and the Humus plug-in share: the theme and its
# controls, the settings file, Appearance settings and the add picker. Kept
# free of EngineHost so a plug-in can link it; hum_host links it for the app,
# so hum_gui, hum_snapshot and hum_tests all get it from there.
set(HUM_UI_SOURCES
  src/gui/style/LookAndFeel.cpp
  src/gui/style/LookAndFeelSliders.cpp
  src/gui/style/LookAndFeelWidgets.cpp
  src/gui/style/HandCursors.cpp
  src/gui/editor/Skin.cpp
  src/gui/editor/juce/JuceBasicBrickViews.cpp
  src/gui/editor/FaceplateView.cpp
  src/gui/app/AppSettings.cpp
  src/gui/settings/Appearance.cpp
  src/gui/settings/AppearanceSettingsView.cpp
  src/gui/settings/AppearanceThemes.cpp
  src/gui/patcher/ModernPicker.cpp
  src/gui/patcher/PatchView.cpp
  src/gui/patcher/PatchViewGeometry.cpp
  src/gui/patcher/PatchViewHits.cpp
  src/gui/patcher/PatchViewPaint.cpp
)
if(APPLE)
  list(APPEND HUM_UI_SOURCES src/gui/style/HandCursors.mm)
endif()
add_library(hum_ui STATIC ${HUM_UI_SOURCES})
target_include_directories(hum_ui PUBLIC src ${HUM_GENERATED_DIR})
target_link_libraries(hum_ui PUBLIC hum_core hum_assets)
target_compile_definitions(hum_ui PRIVATE JUCE_WEB_BROWSER=0)
hum_warnings(hum_ui)
if(TARGET hum_tests AND NOT HUM_GUI)
  target_link_libraries(hum_tests PRIVATE hum_ui)
endif()
