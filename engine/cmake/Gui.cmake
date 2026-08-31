option(HUM_GUI "Build the JUCE GUI app" ON)
if(HUM_GUI)
  juce_add_gui_app(hum_gui PRODUCT_NAME "Humus"
    COMPANY_NAME "Phobos Instruments"
    COMPANY_WEBSITE "https://humus.phobos-instruments.com"
    BUNDLE_ID "com.phobos-instruments.humus"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/icon-1024.png"
    # Library validation would refuse every .humpack and hosted plugin;
    # disabling it is what every plugin host declares (docs/dev/build.md).
    HARDENED_RUNTIME_ENABLED TRUE
    # The usage strings below are NOT enough under the hardened runtime:
    # without the entitlement macOS never shows the prompt at all.
    HARDENED_RUNTIME_OPTIONS "com.apple.security.cs.disable-library-validation"
                             "com.apple.security.device.camera"
                             "com.apple.security.device.audio-input"
                             "com.apple.security.device.bluetooth"
    CAMERA_PERMISSION_ENABLED TRUE
    CAMERA_PERMISSION_TEXT "Humus uses the camera for CamIn organisms - motion and pose become patchable control signals. Video never leaves the app."
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Humus uses your audio input so SoundIn and the recorders can capture a microphone, instrument or line signal. Audio stays on your machine."
    # Without this macOS KILLS the process on the pairing button.
    BLUETOOTH_PERMISSION_ENABLED TRUE
    BLUETOOTH_PERMISSION_TEXT "Humus uses Bluetooth to find and pair wireless MIDI controllers and keyboards. Nothing is sent anywhere."
    # ATS exception (LAN AI endpoints are plain http), plus the .hum document
    # type - BOTH halves, UTI and document, or a double-click never arrives.
    # Only .hum is claimed, never .amh (docs/dev/build.md, "The GUI app").
    PLIST_TO_MERGE [[<plist version="1.0"><dict>
      <key>NSAppTransportSecurity</key>
      <dict><key>NSAllowsArbitraryLoads</key><true/></dict>
      <key>NSLocalNetworkUsageDescription</key>
      <string>Humus connects to your AI server on the local network (Settings → AI) for the assistant, preset and sample features.</string>
      <key>UTExportedTypeDeclarations</key>
      <array><dict>
        <key>UTTypeIdentifier</key><string>com.totel.humus.patch</string>
        <key>UTTypeDescription</key><string>Humus Patch</string>
        <key>UTTypeIconFile</key><string>Icon.icns</string>
        <key>UTTypeConformsTo</key><array><string>public.xml</string></array>
        <key>UTTypeTagSpecification</key>
        <dict><key>public.filename-extension</key><array><string>hum</string></array></dict>
      </dict></array>
      <key>CFBundleDocumentTypes</key>
      <array>
        <dict>
          <key>CFBundleTypeName</key><string>Humus Patch</string>
          <key>CFBundleTypeIconFile</key><string>Icon.icns</string>
          <key>CFBundleTypeRole</key><string>Editor</string>
          <key>LSHandlerRank</key><string>Owner</string>
          <key>LSItemContentTypes</key><array><string>com.totel.humus.patch</string></array>
        </dict>
      </array>
    </dict></plist>]])
  target_sources(hum_gui PRIVATE
    src/gui/Main.cpp
    ${HUM_GUI_SOURCES}
  )
  target_include_directories(hum_gui PRIVATE src ${HUM_GENERATED_DIR})
  add_dependencies(hum_gui hum_build_id)
  target_link_libraries(hum_gui PRIVATE
    hum_core
    hum_assets
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_devices
    juce::juce_audio_utils
    juce::juce_video        # camera names for "video-inputs" combos
    juce::juce_opengl       # Lumen's visual window (gui/VisualWindow.h)
    juce::juce_cryptography # SHA-256 for the update download (gui/AppUpdater.h)
  )
  if(APPLE)
    # Lumen's clip layers (VideoLayerMac.mm): AVFoundation decode -> GL.
    target_link_libraries(hum_gui PRIVATE
      "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")
  endif()
  target_compile_definitions(hum_gui PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="Humus"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
  )

  # Builtin packs travel beside the binary; on macOS as Resources (data) or
  # codesign refuses to seal. packsRootDir() resolves both.
  if(APPLE)
    set(HUM_GUI_PACKS_DIR "$<TARGET_BUNDLE_CONTENT_DIR:hum_gui>/Resources/packs")
  else()
    set(HUM_GUI_PACKS_DIR "$<TARGET_FILE_DIR:hum_gui>/packs")
  endif()
  # A stamped command depending on the files, NOT a POST_BUILD - those run
  # only when the executable relinks, and shipped stale bundles.
  file(GLOB_RECURSE HUM_BUNDLED_PACK_FILES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/../packs/core/*"
       "${CMAKE_CURRENT_SOURCE_DIR}/../packs/humus/*"
       "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av/*")
  file(GLOB_RECURSE HUM_BUNDLED_ASSET_FILES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/../assets/*")

  # assets/ ships whole: bundle, repo and Documents/Humus share one shape.
  if(APPLE)
    set(HUM_GUI_ASSETS_DIR "$<TARGET_BUNDLE_CONTENT_DIR:hum_gui>/Resources/assets")
  else()
    set(HUM_GUI_ASSETS_DIR "$<TARGET_FILE_DIR:hum_gui>/assets")
  endif()
  # The dev tree's own jams (my-*.hum) are staged and removed again.
  file(GLOB HUM_PRIVATE_PATCHES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/../assets/patches/my-*")
  set(HUM_PRIVATE_STRIP_CMD "")
  foreach(f ${HUM_PRIVATE_PATCHES})
    get_filename_component(n "${f}" NAME)
    list(APPEND HUM_PRIVATE_STRIP_CMD
         COMMAND ${CMAKE_COMMAND} -E rm -f "${HUM_GUI_ASSETS_DIR}/patches/${n}")
  endforeach()

  # Signed by hand: Ninja ignores the XCODE_ATTRIBUTE_* wiring JUCE relies
  # on, and signing must follow staging - it seals the bundle's contents.
  set(HUM_GUI_SIGN_CMD "")
  if(APPLE)
    set(HUM_GUI_ENTITLEMENTS
        "${CMAKE_CURRENT_BINARY_DIR}/hum_gui_artefacts/JuceLibraryCode/hum_gui.entitlements")
    # A secure timestamp needs a real identity; ad-hoc must opt out.
    set(HUM_TS "--timestamp")
    if(HUM_CODESIGN_IDENTITY STREQUAL "-")
      set(HUM_TS "--timestamp=none")
    endif()
    set(HUM_GUI_SIGN_CMD
        COMMAND codesign --force --options runtime ${HUM_TS}
                --entitlements "${HUM_GUI_ENTITLEMENTS}"
                --sign "${HUM_CODESIGN_IDENTITY}"
                "$<TARGET_BUNDLE_DIR:hum_gui>")
  endif()

  set(HUM_GUI_BUNDLE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/hum_gui_bundle.stamp")
  add_custom_command(OUTPUT "${HUM_GUI_BUNDLE_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${HUM_GUI_PACKS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../packs/core" "${HUM_GUI_PACKS_DIR}/core"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../packs/humus" "${HUM_GUI_PACKS_DIR}/humus"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av" "${HUM_GUI_PACKS_DIR}/av"
    # Strip pack sources by extension; the manifests beside them survive.
    COMMAND ${CMAKE_COMMAND} -DDIR=${HUM_GUI_PACKS_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_pack_sources.cmake"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${HUM_GUI_ASSETS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../assets" "${HUM_GUI_ASSETS_DIR}"
    COMMAND ${CMAKE_COMMAND} -DDIR=${HUM_GUI_ASSETS_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_junk.cmake"
    ${HUM_PRIVATE_STRIP_CMD}
    ${HUM_GUI_SIGN_CMD}
    COMMAND ${CMAKE_COMMAND} -E touch "${HUM_GUI_BUNDLE_STAMP}"
    DEPENDS hum_gui "$<TARGET_FILE:hum_gui>"
            ${HUM_BUNDLED_PACK_FILES} ${HUM_BUNDLED_ASSET_FILES}
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_pack_sources.cmake"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_junk.cmake"
            # An edited entitlements file must re-seal the bundle.
            ${HUM_GUI_ENTITLEMENTS}
    COMMENT "Bundling packs + assets and sealing Humus.app (identity: ${HUM_CODESIGN_IDENTITY})")
  add_custom_target(hum_gui_bundle ALL DEPENDS "${HUM_GUI_BUNDLE_STAMP}")
endif()
