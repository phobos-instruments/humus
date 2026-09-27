# Shared between hum_gui and hum_snapshot (which mirrors the GUI headlessly).
set(HUM_GUI_SOURCES
    src/gui/app/MainComponent.cpp
    src/gui/app/MainComponentDock.cpp
    src/gui/app/MainComponentMenu.cpp
    src/gui/app/MainComponentFiles.cpp
    src/gui/app/MainComponentAutosave.cpp
    src/gui/app/MainComponentHistory.cpp
    src/gui/app/MainComponentLifecycle.cpp
    src/gui/app/MainComponentTransport.cpp
    src/gui/app/MainComponentBrowser.cpp
    src/gui/app/MainComponentWindows.cpp
    src/gui/plugins/EmbeddedPluginView.cpp
    src/gui/app/MainComponentPluginUI.cpp
    src/gui/app/MainComponentLayout.cpp
    src/gui/patcher/PatcherCanvas.cpp
    src/gui/patcher/PatcherCanvasEdit.cpp
    src/gui/patcher/PatcherCanvasFileDrop.cpp
    src/gui/patcher/PatcherCanvasGeometry.cpp
    src/gui/patcher/PatcherCanvasPorts.cpp
    src/gui/video/X11Mirror.cpp
    src/gui/video/X11MirrorInput.cpp
    src/gui/patcher/PatcherCanvasPaint.cpp
    src/gui/patcher/PatcherCanvasArrange.cpp
    src/gui/patcher/PatcherCanvasMouse.cpp
    src/gui/patcher/PatcherCanvasMenus.cpp
    src/gui/pianoroll/PianoRollEditor.cpp
    src/gui/pianoroll/PianoRollColour.cpp
    src/gui/pianoroll/PianoRollGrid.cpp
    src/gui/pianoroll/PianoRollInput.cpp
    src/gui/pianoroll/PianoRollLane.cpp
    src/gui/pianoroll/PianoRollTools.cpp
    src/gui/pianoroll/PianoRollRecord.cpp
    src/gui/tracks/TracksPane.cpp
    src/gui/tracks/ZoomBar.cpp
    src/gui/tracks/TimelineRuler.cpp
    src/gui/tracks/ClipEditorView.cpp
    src/gui/tracks/ClipEditorEdits.cpp
    src/gui/tracks/ClipEditorPaint.cpp
    src/gui/tracks/ClipEditorTransients.cpp
    src/gui/tracks/TrackRollView.cpp
    src/gui/tracks/TrackRollMenu.cpp
    src/gui/tracks/TrackRollEdit.cpp
    src/gui/tracks/TrackRollPaint.cpp
    src/gui/tracks/TrackRollKeys.cpp
    src/gui/tracks/TracksPaneCrumbs.cpp
    src/gui/tracks/TracksPaneKeys.cpp
    src/gui/tracks/TracksPaneModes.cpp
    src/gui/tracks/TracksPaneScroll.cpp
    src/gui/tracks/SongView.cpp
    src/gui/tracks/SongViewAutoLane.cpp
    src/gui/tracks/SongViewBody.cpp
    src/gui/tracks/SongViewBoxPaint.cpp
    src/gui/tracks/SongViewClip.cpp
    src/gui/tracks/SongViewClipActions.cpp
    src/gui/tracks/SongViewDrag.cpp
    src/gui/tracks/SongViewFileDrop.cpp
    src/gui/tracks/SongViewFramestrip.cpp
    src/gui/tracks/SongViewGeometry.cpp
    src/gui/tracks/SongViewHeader.cpp
    src/gui/tracks/SongViewMidiInput.cpp
    src/gui/tracks/SongViewInput.cpp
    src/gui/tracks/SongViewItems.cpp
    src/gui/tracks/SongViewKeys.cpp
    src/gui/tracks/SongViewMenu.cpp
    src/gui/tracks/SongViewMove.cpp
    src/gui/tracks/SongViewPaint.cpp
    src/gui/tracks/SongViewRepeat.cpp
    src/gui/tracks/SongViewRowPaint.cpp
    src/gui/tracks/SongViewSelect.cpp
    src/gui/tracks/SongViewPointOps.cpp
    src/gui/tracks/SongViewRowOrder.cpp
    src/gui/editor/ParameterPanel.cpp
    src/gui/editor/OrganismEditorFactory.cpp
    src/gui/properties/PropertiesPane.cpp
    src/gui/style/HandCursors.cpp
    src/gui/editor/Skin.cpp
    src/gui/editor/QuickMapDialog.cpp
    src/gui/app/WindowMinimum.cpp
    src/gui/properties/PropertiesPaneLayout.cpp
    src/gui/properties/ParameterWindow.cpp
    src/gui/app/MainComponentStatus.cpp
    src/gui/editor/juce/JuceBrickViews.cpp
    src/gui/editor/juce/JuceRichPlayers.cpp
    src/gui/editor/juce/JuceRichReadouts.cpp
    src/gui/editor/juce/JuceRichPickers.cpp
    src/gui/app/MainComponentTransportMenus.cpp
    src/gui/patcher/PatcherCanvasPodMenus.cpp
    src/gui/properties/ParameterWindowInput.cpp
    src/gui/properties/ParameterWindowLayout.cpp
    src/gui/patcher/PatcherCanvasDragHints.cpp
    src/gui/settings/AppearanceThemes.cpp
    src/gui/help/HelpBrowser.cpp
    src/gui/properties/MetapadView.cpp
    src/gui/properties/PresetRail.cpp
    src/gui/bricks/ClipCell.cpp
    src/gui/bricks/ClipGridBrick.cpp
    src/gui/settings/MidiSettingsView.cpp
    src/gui/patcher/ModernPicker.cpp
    src/gui/bricks/WaveDrawBrick.cpp
    src/gui/properties/ParameterControlView.cpp
    src/gui/properties/ParameterControlRows.cpp
    src/gui/properties/ParameterControlInspect.cpp
    src/gui/properties/MappingInspector.cpp
    src/gui/properties/ParameterControlGroups.cpp
    src/gui/video/VisualGlCanvasPrograms.cpp
    src/gui/video/VisualGlCanvasScenes.cpp
    src/gui/video/VisualGlCanvasDecks.cpp
    src/gui/video/VisualGlCanvasTaps.cpp
    src/gui/video/VisualGlCanvas.cpp
    src/gui/style/LookAndFeel.cpp
    src/gui/app/DefaultPatchHandler.cpp
    src/gui/settings/SettingsComponent.cpp
    src/gui/settings/AiSettingsView.cpp
    src/gui/settings/AppearanceSettingsView.cpp
    src/gui/app/SetupWizard.cpp
    src/gui/style/LookAndFeelSliders.cpp
    src/gui/style/LookAndFeelWidgets.cpp
    src/gui/editor/LayoutEditor.cpp
    src/gui/bricks/PatternEditor.cpp
    src/gui/bricks/PatternEditorInput.cpp
)

# The EngineHost and its GUI-free helpers, built once as hum_host (cmake/Host.cmake).
set(HUM_HOST_SOURCES
    src/gui/assistant/AssistantEngine.cpp
    src/gui/assistant/AssistantAutomix.cpp
    src/gui/assistant/AssistantTools.cpp
    src/gui/video/VideoLayer.cpp
    src/gui/video/VideoEncoder.cpp
    src/gui/video/VisualPlanBuilder.cpp
    src/gui/video/VisualPlanSteps.cpp
    src/gui/host/HostAudioIo.cpp
    src/gui/host/HostScheduler.cpp
    src/gui/host/HostHistory.cpp
    src/gui/host/GamepadHost.cpp
    src/gui/browser/Audition.cpp
    src/gui/browser/BrowserInspector.cpp
    src/gui/browser/FileBrowserActions.cpp
    src/gui/browser/FileBrowserMenus.cpp
    src/gui/browser/FileBrowserPanels.cpp
    src/gui/browser/LibraryActions.cpp
    src/gui/browser/Loaders.cpp
    src/gui/browser/FileBrowserView.cpp
    src/gui/browser/SoundProbe.cpp
    src/gui/host/EngineHostBounce.cpp
    src/gui/host/EngineHostAudioTakes.cpp
    src/gui/host/EngineHost.cpp
    src/gui/host/EngineHostPodImport.cpp
    src/gui/host/EngineHostEdit.cpp
    src/gui/host/EngineHostEditCords.cpp
    src/gui/host/EngineHostParams.cpp
    src/gui/host/ControlApply.cpp
    src/gui/host/EngineHostControlSummary.cpp
    src/gui/host/EngineHostGroove.cpp
    src/gui/host/EngineHostPresets.cpp
    src/gui/host/EngineHostRecord.cpp
    src/gui/host/EngineHostFiles.cpp
    src/gui/host/EngineHostDeck.cpp
    src/gui/host/EngineHostMedia.cpp
    src/gui/host/EngineHostMetapad.cpp
    src/gui/host/EngineHostMidi.cpp
    src/gui/host/EngineHostMidiControl.cpp
    src/gui/host/EngineHostRollRecord.cpp
    src/gui/host/EngineHostMidiGraph.cpp
    src/gui/host/EngineHostSync.cpp
    src/gui/host/EngineHostLink.cpp
    src/gui/host/LinkSync.cpp
    src/gui/host/EngineHostAutomation.cpp
    src/gui/host/EngineHostSequencer.cpp
    src/gui/host/EngineHostAudio.cpp
    src/gui/host/EngineHostTriggers.cpp
    src/gui/host/EngineHostClipNotes.cpp
    src/gui/host/EngineHostClipOps.cpp
    src/gui/host/EngineHostCapture.cpp
    src/gui/host/EngineHostTouch.cpp
    src/gui/host/EngineHostChoices.cpp
    src/gui/host/EngineHostControlCords.cpp
    src/gui/host/EngineHostDevice.cpp
    src/gui/host/EngineHostDocument.cpp
    src/gui/host/EngineHostInsert.cpp
    src/gui/host/EngineHostLiveSync.cpp
    src/gui/host/EngineHostMidiCords.cpp
    src/gui/host/EngineHostNodes.cpp
    src/gui/host/EngineHostOffline.cpp
    src/gui/host/EngineHostPodClipboard.cpp
    src/gui/host/EngineHostPods.cpp
    src/gui/host/EngineHostPseudo.cpp
    src/gui/host/EngineHostRetro.cpp
    src/gui/host/EngineHostInputReach.cpp
    src/gui/host/EngineHostTakeNotes.cpp
    src/gui/host/EngineHostTransport.cpp
    src/gui/host/EngineHostUndo.cpp
    src/gui/host/EngineHostViews.cpp
    src/gui/host/EngineHostPattern.cpp
    src/gui/host/EngineHostClips.cpp
    src/gui/host/EngineHostPlugins.cpp
    src/gui/app/AppSettings.cpp
    src/gui/host/OscHost.cpp
    src/gui/settings/OscSerial.cpp
    src/gui/host/ModHost.cpp
)

# Video decode: the OS framework where there is one (AVFoundation, Media
# Foundation), FFmpeg's LGPL libraries on Linux via pkg-config - the system's,
# or the trimmed build packaging/linux/ffmpeg-lite.sh makes (HUM_FFMPEG_ROOT).
# Without any, VideoPlayer plays HAP videos only; the configure says so.
#
# Video encode (what Bounce writes): the same OS frameworks, which is why
# macOS and Windows ship no encoder of their own and nothing under the GPL -
# HUM_VIDEO_NATIVE picks VideoEncoderMac.mm / VideoEncoderWin.cpp. Linux has
# no system encoder, so it goes through the FFmpeg above, whose x264 is the
# reason ffmpeg-lite.sh is a GPL build.
add_library(hum_video INTERFACE)
target_link_libraries(hum_video INTERFACE hum_ffmpeg)


if(APPLE)
  list(APPEND HUM_HOST_SOURCES src/gui/video/VideoLayerMac.mm src/gui/video/VideoEncoderMac.mm)
  target_link_libraries(hum_video INTERFACE
    "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")
  target_compile_definitions(hum_video INTERFACE HUM_VIDEO_NATIVE=1)
  message(STATUS "video encode: AVFoundation (H.264 through the system encoder)")
elseif(WIN32)
  list(APPEND HUM_HOST_SOURCES src/gui/video/VideoLayerMediaFoundation.cpp src/gui/video/VideoEncoderWin.cpp)
  target_link_libraries(hum_video INTERFACE mfplat mfreadwrite mfuuid ole32)
  target_compile_definitions(hum_video INTERFACE HUM_MEDIA_FOUNDATION=1 HUM_VIDEO_NATIVE=1)
  message(STATUS "video decode: Media Foundation")
  message(STATUS "video encode: Media Foundation (H.264 through the system encoder)")
endif()

if(HUM_FFMPEG_FOUND)
  list(APPEND HUM_HOST_SOURCES src/gui/video/VideoLayerFfmpeg.cpp)
  message(STATUS "video decode: the system's, with FFmpeg behind it for what it cannot open")
elseif(NOT APPLE AND NOT WIN32)
  message(WARNING
    "no FFmpeg development files (libavcodec-dev libavformat-dev libswscale-dev "
    "libswresample-dev): VideoPlayer will play HAP videos only in this build, and "
    "no video will give up its sound.")
endif()

# The rpath is a target property, not a link option: Ninja eats the $ORIGIN.
function(hum_link_video target)
  target_link_libraries(${target} PRIVATE hum_video)
  if(HUM_FFMPEG_FOUND AND HUM_FFMPEG_ROOT)
    set_property(TARGET ${target} APPEND PROPERTY BUILD_RPATH
      "\$ORIGIN/lib" "${HUM_FFMPEG_ROOT}/lib")
  endif()
endfunction()

# hum_tests is created in Tests.cmake, which is included BEFORE this file, so
# the link belongs here rather than there. VideoHwAccel.h is FFmpeg's own
# header: without this the suite that covers it compiled only where libavutil
# happened to sit in /usr/include, and broke the Windows build instead.
if(TARGET hum_tests)
  hum_link_video(hum_tests)
endif()

if(APPLE)
  list(APPEND HUM_HOST_SOURCES src/gui/host/GamepadHostMac.mm)
  list(APPEND HUM_GUI_SOURCES
    src/gui/plugins/EmbeddedPluginViewMac.mm
    src/gui/plugins/PluginEditorWindowMac.mm
    src/gui/style/HandCursors.mm
    src/gui/app/AppNapMac.mm
    src/gui/tracks/FileDragImageMac.mm
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
