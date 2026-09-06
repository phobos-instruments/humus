# Shared between hum_gui and hum_snapshot (which mirrors the GUI headlessly).
set(HUM_GUI_SOURCES
    src/gui/MainComponent.cpp
    src/gui/MainComponentDock.cpp
    src/gui/MainComponentMenu.cpp
    src/gui/MainComponentFiles.cpp
    src/gui/MainComponentAutosave.cpp
    src/gui/MainComponentLifecycle.cpp
    src/gui/MainComponentTransport.cpp
    src/gui/MainComponentWindows.cpp
    src/gui/EmbeddedPluginView.cpp
    src/gui/GamepadHost.cpp
    src/gui/VideoLayer.cpp
    src/gui/MainComponentPluginUI.cpp
    src/gui/MainComponentLayout.cpp
    src/gui/PatcherCanvas.cpp
    src/gui/PatcherCanvasEdit.cpp
    src/gui/PatcherCanvasGeometry.cpp
    src/gui/X11Mirror.cpp
    src/gui/X11MirrorInput.cpp
    src/gui/PatcherCanvasPaint.cpp
    src/gui/PatcherCanvasArrange.cpp
    src/gui/PatcherCanvasMouse.cpp
    src/gui/PatcherCanvasMenus.cpp
    src/gui/PianoRollEditor.cpp
    src/gui/PianoRollGrid.cpp
    src/gui/PianoRollInput.cpp
    src/gui/PianoRollLane.cpp
    src/gui/PianoRollTools.cpp
    src/gui/TracksPane.cpp
    src/gui/TracksPaneInput.cpp
    src/gui/EngineHostBounce.cpp
    src/gui/TracksPaneClip.cpp
    src/gui/TracksPaneClipPaint.cpp
    src/gui/TracksPaneMenu.cpp
    src/gui/TracksPaneRepeat.cpp
    src/gui/TracksPaneRoll.cpp
    src/gui/TracksPaneRollEdit.cpp
    src/gui/TracksPaneMove.cpp
    src/gui/TracksPaneSelect.cpp
    src/gui/TracksPaneItems.cpp
    src/gui/TracksPanePaint.cpp
    src/gui/ParameterPanel.cpp
    src/gui/PropertiesPane.cpp
    src/gui/PropertiesPaneLayout.cpp
    src/gui/ParameterWindow.cpp
    src/gui/EngineHost.cpp
    src/gui/EngineHostPodImport.cpp
    src/gui/EngineHostEdit.cpp
    src/gui/EngineHostEditCords.cpp
    src/gui/EngineHostParams.cpp
    src/gui/EngineHostPresets.cpp
    src/gui/EngineHostRecord.cpp
    src/gui/EngineHostFiles.cpp
    src/gui/EngineHostDeck.cpp
    src/gui/EngineHostMetapad.cpp
    src/gui/EngineHostMidi.cpp
    src/gui/EngineHostMidiControl.cpp
    src/gui/EngineHostMidiGraph.cpp
    src/gui/EngineHostSync.cpp
    src/gui/EngineHostLink.cpp
    src/gui/LinkSync.cpp
    src/gui/EngineHostAutomation.cpp
    src/gui/EngineHostSequencer.cpp
    src/gui/EngineHostAudio.cpp
    src/gui/EngineHostPattern.cpp
    src/gui/EngineHostClips.cpp
    src/gui/EngineHostPlugins.cpp
    src/gui/LookAndFeel.cpp
    src/gui/AppSettings.cpp
    src/gui/DefaultPatchHandler.cpp
    src/gui/SettingsComponent.cpp
    src/gui/SettingsComponentAi.cpp
    src/gui/SetupWizard.cpp
    src/gui/OscHost.cpp
    src/gui/ModHost.cpp
    src/gui/LookAndFeelSliders.cpp
    src/gui/LookAndFeelWidgets.cpp
    src/gui/LayoutLoader.cpp
    src/gui/LayoutEditor.cpp
    src/gui/LayoutEditorBricks.cpp
    src/gui/PatternEditor.cpp
    src/gui/PatternEditorInput.cpp
)

# Video decode: the OS framework where there is one (AVFoundation, Media
# Foundation), FFmpeg's LGPL libraries on Linux via pkg-config - the system's,
# or the trimmed build packaging/linux/ffmpeg-lite.sh makes (HUM_FFMPEG_ROOT).
# Without any, VideoPlayer plays HAP videos only; the configure says so.
option(HUM_FFMPEG "Play videos in VideoPlayer through FFmpeg on Linux" ON)
set(HUM_FFMPEG_ROOT "" CACHE PATH
  "Prefix of a private FFmpeg build to link and bundle; empty = the system's")
set(HUM_FFMPEG_FOUND OFF)
add_library(hum_video INTERFACE)
if(APPLE)
  list(APPEND HUM_GUI_SOURCES src/gui/VideoLayerMac.mm)
  target_link_libraries(hum_video INTERFACE
    "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")
elseif(WIN32)
  list(APPEND HUM_GUI_SOURCES src/gui/VideoLayerMediaFoundation.cpp)
  target_link_libraries(hum_video INTERFACE mfplat mfreadwrite mfuuid ole32)
  target_compile_definitions(hum_video INTERFACE HUM_MEDIA_FOUNDATION=1)
  message(STATUS "video decode: Media Foundation")
elseif(HUM_FFMPEG)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    if(HUM_FFMPEG_ROOT)
      set(ENV{PKG_CONFIG_PATH} "${HUM_FFMPEG_ROOT}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
    # Re-probed every configure, or a moved HUM_FFMPEG_ROOT keeps the old answer.
    unset(HUM_FFMPEG_LIBS_FOUND CACHE)
    foreach(lib avcodec avformat avutil swscale)
      unset(pkgcfg_lib_HUM_FFMPEG_LIBS_${lib} CACHE)
    endforeach()
    pkg_check_modules(HUM_FFMPEG_LIBS QUIET IMPORTED_TARGET
      libavcodec libavformat libavutil libswscale)
  endif()
  if(HUM_FFMPEG_LIBS_FOUND)
    set(HUM_FFMPEG_FOUND ON)
    list(APPEND HUM_GUI_SOURCES src/gui/VideoLayerFfmpeg.cpp)
    target_link_libraries(hum_video INTERFACE PkgConfig::HUM_FFMPEG_LIBS)
    target_compile_definitions(hum_video INTERFACE HUM_FFMPEG=1)
    message(STATUS "video decode: FFmpeg libavcodec ${HUM_FFMPEG_LIBS_libavcodec_VERSION}")
  else()
    message(WARNING
      "no FFmpeg development files (libavcodec-dev libavformat-dev libswscale-dev): "
      "VideoPlayer will play HAP videos only in this build.")
  endif()
endif()

# The rpath is a target property, not a link option: Ninja eats the $ORIGIN.
function(hum_link_video target)
  target_link_libraries(${target} PRIVATE hum_video)
  if(HUM_FFMPEG_FOUND AND HUM_FFMPEG_ROOT)
    set_property(TARGET ${target} APPEND PROPERTY BUILD_RPATH
      "\$ORIGIN/lib" "${HUM_FFMPEG_ROOT}/lib")
  endif()
endfunction()

if(APPLE)
  list(APPEND HUM_GUI_SOURCES
    src/gui/EmbeddedPluginViewMac.mm
    src/gui/PluginEditorWindowMac.mm
    src/gui/MacCursors.mm
    src/gui/GamepadHostMac.mm
    src/gui/AppNapMac.mm
    src/gui/FileDragImageMac.mm
  )
endif()

# Outside the HUM_GUI guard: hum_tests reads the tour copy too.
file(GLOB HUM_EMBEDDED_ART CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/glyphs/*.svg"
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/glyphs/*.png")
file(GLOB HUM_EMBEDDED_GUIDE CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/guide/*.md")
juce_add_binary_data(hum_assets SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/icon-1024.png"
  ${HUM_EMBEDDED_ART}
  ${HUM_EMBEDDED_GUIDE})
