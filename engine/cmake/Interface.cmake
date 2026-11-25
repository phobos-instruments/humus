# The app's interface, compiled once. hum_interface is everything the app and
# the Humus plug-in both show: the patcher, the properties pane and its editor
# parts, the piano roll, tracks, settings, help and the hosted plug-in views.
# hum_shell adds what only the app has on top: its main window, menus, setup
# wizard and What's New. hum_gui and hum_snapshot link hum_shell; the plug-in
# links hum_interface. Nothing in these sources registers itself at startup,
# so taking them from static archives drops nothing.
if(HUM_GUI AND TARGET hum_host)
  set(HUM_SHELL_PATTERN "src/gui/app/(Main|WhatsNew|SetupWizard|DefaultPatch|AppNap|MenuBar)")
  set(HUM_INTERFACE_SOURCES ${HUM_GUI_SOURCES})
  list(FILTER HUM_INTERFACE_SOURCES EXCLUDE REGEX "${HUM_SHELL_PATTERN}")
  set(HUM_SHELL_SOURCES ${HUM_GUI_SOURCES})
  list(FILTER HUM_SHELL_SOURCES INCLUDE REGEX "${HUM_SHELL_PATTERN}")

  add_library(hum_interface STATIC ${HUM_INTERFACE_SOURCES})
  target_include_directories(hum_interface PUBLIC src ${HUM_GENERATED_DIR})
  add_dependencies(hum_interface hum_build_id)
  target_link_libraries(hum_interface PUBLIC hum_host hum_core hum_assets)
  target_compile_definitions(hum_interface PRIVATE JUCE_WEB_BROWSER=0)
  hum_warnings(hum_interface)

  add_library(hum_shell STATIC ${HUM_SHELL_SOURCES})
  target_link_libraries(hum_shell PUBLIC hum_interface)
  target_compile_definitions(hum_shell PRIVATE JUCE_WEB_BROWSER=0)
  hum_warnings(hum_shell)

  # The X11 window mirror lives in hum_interface, so its define and libraries
  # go there and reach the app and the snapshot tool through it.
  if(UNIX AND NOT APPLE)
    find_package(X11 COMPONENTS Xcomposite Xext)
    if(X11_FOUND AND X11_Xcomposite_FOUND)
      target_link_libraries(hum_interface PUBLIC X11::X11 X11::Xcomposite X11::Xext)
      target_compile_definitions(hum_interface PRIVATE HUM_X11_MIRROR=1)
    endif()
  endif()
endif()
