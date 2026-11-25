# CLAP hosting, the JUCE-free half: loading a .clap file, the host callbacks
# the plug-in calls, event lists, state streams and the transport event. The
# standard is the vendored C headers in third_party/clap (MIT); there is no
# library to link. The JUCE side (the plug-in format, the instance, its
# window) is in src/gui/plugins/clap and links this.
add_library(hum_clap_headers INTERFACE)
target_include_directories(hum_clap_headers SYSTEM INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/clap/include)

add_library(hum_clap STATIC
  src/core/plugins/clap/ClapEvents.cpp
  src/core/plugins/clap/ClapHost.cpp
  src/core/plugins/clap/ClapLoad.cpp
  src/core/plugins/clap/ClapModule.cpp
  src/core/plugins/clap/ClapNotes.cpp
)
target_include_directories(hum_clap PUBLIC
  src
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include)
target_link_libraries(hum_clap PUBLIC hum_clap_headers ${CMAKE_DL_LIBS})
target_compile_features(hum_clap PUBLIC cxx_std_17)
if(APPLE)
  target_link_libraries(hum_clap PUBLIC "-framework CoreFoundation")
endif()

# The JUCE side: CLAP as a juce::AudioPluginFormat beside VST3 and AU, its
# instances and their windows. Every program that hosts plug-ins calls
# hum::claphost::enableClapHosting() once at start: the app (and the child
# processes it starts for scanning and bridging), the Humus plug-in, the CLI,
# hum_tests and hum_snapshot.
add_library(hum_clap_juce STATIC
  src/gui/plugins/clap/ClapEditor.cpp
  src/gui/plugins/clap/ClapFormat.cpp
  src/gui/plugins/clap/ClapInstance.cpp
  src/gui/plugins/clap/ClapNativeParent.cpp
  src/gui/plugins/clap/ClapParameter.cpp
  src/gui/plugins/clap/ClapProcess.cpp
  src/gui/plugins/clap/ClapSession.cpp
  src/gui/plugins/clap/ClapTimers.cpp
)
if(APPLE)
  target_sources(hum_clap_juce PRIVATE src/gui/plugins/clap/ClapNativeParentMac.mm)
endif()
target_include_directories(hum_clap_juce PRIVATE ${HUM_GENERATED_DIR})
add_dependencies(hum_clap_juce hum_build_id)
target_link_libraries(hum_clap_juce PUBLIC hum_core hum_clap)
