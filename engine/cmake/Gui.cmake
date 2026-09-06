option(HUM_GUI "Build the JUCE GUI app" ON)
if(HUM_GUI)
  juce_add_gui_app(hum_gui PRODUCT_NAME "Humus"
    COMPANY_NAME "Phobos Instruments"
    COMPANY_WEBSITE "https://humus.phobos-instruments.com"
    BUNDLE_ID "com.phobos-instruments.humus"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/icon-1024.png"
    # No library validation: every .humpack and hosted plugin would be refused.
    # The device entitlements gate the permission prompts (docs/dev/build.md).
    HARDENED_RUNTIME_ENABLED TRUE
    HARDENED_RUNTIME_OPTIONS "com.apple.security.cs.disable-library-validation"
                             "com.apple.security.device.camera"
                             "com.apple.security.device.audio-input"
                             "com.apple.security.device.bluetooth"
    CAMERA_PERMISSION_ENABLED TRUE
    CAMERA_PERMISSION_TEXT "Humus uses the camera for CamIn organisms - motion and pose become patchable control signals. Video never leaves the app."
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Humus uses your audio input so SoundIn and the recorders can capture a microphone, instrument or line signal. Audio stays on your machine."
    BLUETOOTH_PERMISSION_ENABLED TRUE
    BLUETOOTH_PERMISSION_TEXT "Humus uses Bluetooth to find and pair wireless MIDI controllers and keyboards. Nothing is sent anywhere."
    # ATS exception for plain-http LAN AI endpoints; the .hum type needs both
    # the UTI and the document entry (docs/dev/build.md, "The GUI app").
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
    juce::juce_video
    juce::juce_opengl
    juce::juce_cryptography
  )
  hum_link_video(hum_gui)
  target_compile_definitions(hum_gui PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_APPLICATION_NAME_STRING="Humus"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
  )

  # On macOS data lives in Resources or codesign refuses to seal the bundle.
  if(APPLE)
    set(HUM_GUI_PACKS_DIR "$<TARGET_BUNDLE_CONTENT_DIR:hum_gui>/Resources/packs")
  else()
    set(HUM_GUI_PACKS_DIR "$<TARGET_FILE_DIR:hum_gui>/packs")
  endif()
  # A stamped command, not POST_BUILD: that only runs when the exe relinks.
  file(GLOB_RECURSE HUM_BUNDLED_PACK_FILES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/../packs/core/*"
       "${CMAKE_CURRENT_SOURCE_DIR}/../packs/humus/*"
       "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av/*")
  file(GLOB_RECURSE HUM_BUNDLED_ASSET_FILES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/../assets/*")

  if(APPLE)
    set(HUM_GUI_ASSETS_DIR "$<TARGET_BUNDLE_CONTENT_DIR:hum_gui>/Resources/assets")
  else()
    set(HUM_GUI_ASSETS_DIR "$<TARGET_FILE_DIR:hum_gui>/assets")
  endif()
  # The dev tree's own jams (my-*.hum) never ship.
  file(GLOB HUM_PRIVATE_PATCHES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/../assets/patches/my-*")
  set(HUM_PRIVATE_STRIP_CMD "")
  foreach(f ${HUM_PRIVATE_PATCHES})
    get_filename_component(n "${f}" NAME)
    list(APPEND HUM_PRIVATE_STRIP_CMD
         COMMAND ${CMAKE_COMMAND} -E rm -f "${HUM_GUI_ASSETS_DIR}/patches/${n}")
  endforeach()

  # Signed by hand after staging: Ninja ignores JUCE's Xcode signing wiring.
  set(HUM_GUI_SIGN_CMD "")
  if(APPLE)
    set(HUM_GUI_ENTITLEMENTS
        "${CMAKE_CURRENT_BINARY_DIR}/hum_gui_artefacts/JuceLibraryCode/hum_gui.entitlements")
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

  # With the Core ML packages beside it, the .humnet is never read (macOS only).
  set(HUM_GUI_MODEL_STRIP_CMD "")
  if(APPLE)
    set(HUM_GUI_MODEL_STRIP_CMD
        COMMAND ${CMAKE_COMMAND} -DDIR=${HUM_GUI_PACKS_DIR}
                -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_interpreter_models.cmake")
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
    COMMAND ${CMAKE_COMMAND} -DDIR=${HUM_GUI_PACKS_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_pack_sources.cmake"
    COMMAND ${CMAKE_COMMAND} -DDIR=${HUM_GUI_PACKS_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_junk.cmake"
    ${HUM_GUI_MODEL_STRIP_CMD}
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
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_interpreter_models.cmake"
            ${HUM_GUI_ENTITLEMENTS}
    COMMENT "Bundling packs + assets and sealing Humus.app (identity: ${HUM_CODESIGN_IDENTITY})")
  add_custom_target(hum_gui_bundle ALL DEPENDS "${HUM_GUI_BUNDLE_STAMP}")
endif()
