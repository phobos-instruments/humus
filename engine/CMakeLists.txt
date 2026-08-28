cmake_minimum_required(VERSION 3.22)

# Minimum macOS - must be set BEFORE project(), or the compiler checks bake in
# the host SDK's default and the flag does nothing. Big Sur is the first release
# with Apple Silicon, so it is the natural floor: it covers every Mac that runs
# arm64 natively plus Intel hardware back to ~2013. Override on the command line
# (-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15) to reach further back.
# Cross-building for Intel from Apple Silicon: -DCMAKE_OSX_ARCHITECTURES=x86_64
# (or `make build ARCH=x86_64`, which keeps a separate build tree).
# Test the VALUE, not DEFINED: CMake pre-seeds an EMPTY cache entry, so a
# `NOT DEFINED` guard silently does nothing in any existing build tree and the
# host SDK's default wins (15.0 here). A plain variable shadows that empty entry;
# an explicit -DCMAKE_OSX_DEPLOYMENT_TARGET=... is non-empty and still wins.
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

project(Humus VERSION 0.1.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The build id (see cmake/BuildId.cmake): generated once here so the first
# configure leaves a header to include, then refreshed by a target that runs
# on every build so it never lies about which commit you are looking at.
set(HUM_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(HUM_BUILD_ID_H "${HUM_GENERATED_DIR}/HumBuildId.h")
set(HUM_BUILD_ID_ARGS
  -DVERSION=${PROJECT_VERSION}
  -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/..
  -DSTAMP=${CMAKE_CURRENT_SOURCE_DIR}/build-stamp.txt
  -DOUT=${HUM_BUILD_ID_H})
file(MAKE_DIRECTORY "${HUM_GENERATED_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} ${HUM_BUILD_ID_ARGS}
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildId.cmake)
add_custom_target(hum_build_id ALL
  COMMAND ${CMAKE_COMMAND} ${HUM_BUILD_ID_ARGS}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildId.cmake
  BYPRODUCTS ${HUM_BUILD_ID_H}
  COMMENT "Stamping the build id")

if(MSVC)
  # MSVC's <cmath> hides M_PI behind _USE_MATH_DEFINES (every other compiler
  # just provides it), and windows.h's min/max macros break std::min/max.
  # Directory-scoped here so the packs (added below) inherit both.
  add_compile_definitions(_USE_MATH_DEFINES NOMINMAX)
endif()
# Everything compiles PIC so the pack static libs can also link into the
# dynamic .humpack shared libraries.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(HUM_DYN_PACKS "Also build packs as dynamic .humpack payloads" ON)

# Code-signing identity. "-" is ad-hoc: fine on this machine, and enough for
# arm64 (which refuses to execute unsigned code at all), but NOT distributable -
# Gatekeeper quarantines anything downloaded that is not Developer ID signed and
# notarized. Pass a real identity to ship:
#   cmake -B build -DHUM_CODESIGN_IDENTITY="Developer ID Application: You (TEAMID)"
# then `make notarize` (see the Makefile) once per release.
set(HUM_CODESIGN_IDENTITY "-" CACHE STRING
    "codesign identity: '-' = ad-hoc (dev), or 'Developer ID Application: ...'")

set(HUM_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk")
include(${HUM_SDK_DIR}/cmake/HumPack.cmake)

option(HUM_LIVE_AUDIO "Build live audio output (needs ALSA dev headers)" ON)

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG        8.0.4
  GIT_SHALLOW    TRUE
  # Fatal combiner race on macOS (crash at boot with separate in/out
  # devices) - see cmake/PatchJuceCoreAudio.cmake. Idempotent.
  # ...on Windows, D2D draws no images at all - cmake/PatchJuceDirect2D.cmake.
  # ...and on Linux without libcurl, every POST body arrives malformed -
  # cmake/PatchJuceNetworkLinux.cmake.
  # All three edit platform-specific files, so all three run everywhere.
  PATCH_COMMAND  ${CMAKE_COMMAND} -DJUCE_SOURCE_DIR=<SOURCE_DIR>
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchJuceCoreAudio.cmake
         COMMAND ${CMAKE_COMMAND} -DJUCE_SOURCE_DIR=<SOURCE_DIR>
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchJuceDirect2D.cmake
         COMMAND ${CMAKE_COMMAND} -DJUCE_SOURCE_DIR=<SOURCE_DIR>
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchJuceNetworkLinux.cmake
)
FetchContent_MakeAvailable(JUCE)

# JUCE #includes the VST3 SDK's vst2persistence.cpp from a header, so its
# multi-character chunk ids ('CcnK') compile inside our targets where no
# -isystem reaches them. Quiet the file, not the targets, so our own sources
# keep the warning. Link needs no entry - it arrives SYSTEM.
if(NOT MSVC)
  set_source_files_properties(
    "${juce_SOURCE_DIR}/modules/juce_audio_processors/juce_audio_processors.cpp"
    "${juce_SOURCE_DIR}/modules/juce_audio_processors/juce_audio_processors.mm"
    PROPERTIES COMPILE_OPTIONS "-Wno-multichar")
endif()

# ---- inference models, rebuilt when the tree does not carry them ----
# This repository carries assets/Models/hands/hands.humnet; the source
# distribution does not, and rebuilds it here from Google's own bundle. The
# rule for failures: not being able to OBTAIN it warns and carries on, because
# HandsPortable returns no landmarks when it did not load. Bytes that are NOT
# the model stop the configure, because a moved upstream or a tampered mirror
# must not reach a signed app.
set(HUM_HANDS_MODEL "${CMAKE_CURRENT_SOURCE_DIR}/../assets/Models/hands/hands.humnet")
set(HUM_HANDS_SHA256 "e531b99d34cf03a7d668f03c35e9fff48137ae471604f632f83f66e9ee71c522")
set(HUM_HANDS_TASK_SHA256 "fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1")
# The versioned path, not float16/latest/: same bytes today, and only one of
# them promises to stay that way.
set(HUM_HANDS_TASK_URL
    "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
    CACHE STRING "MediaPipe bundle the hand model is converted from")

if(NOT EXISTS "${HUM_HANDS_MODEL}")
  set(HUM_HANDS_WORK "${CMAKE_CURRENT_BINARY_DIR}/hands")
  set(HUM_HANDS_TASK "${HUM_HANDS_WORK}/hand_landmarker.task")
  file(REMOVE_RECURSE "${HUM_HANDS_WORK}")
  file(MAKE_DIRECTORY "${HUM_HANDS_WORK}")
  message(STATUS "hand model not in the tree - rebuilding it from ${HUM_HANDS_TASK_URL}")

  hum_find_python()
  file(DOWNLOAD "${HUM_HANDS_TASK_URL}" "${HUM_HANDS_TASK}"
       TLS_VERIFY ON STATUS HUM_HANDS_STATUS SHOW_PROGRESS)
  list(GET HUM_HANDS_STATUS 0 HUM_HANDS_CODE)
  list(GET HUM_HANDS_STATUS 1 HUM_HANDS_TEXT)

  set(HUM_HANDS_WHY "")
  if(NOT HUM_HANDS_CODE EQUAL 0)
    set(HUM_HANDS_WHY "the bundle could not be downloaded (${HUM_HANDS_TEXT})")
  elseif(NOT HUM_PYTHON)
    set(HUM_HANDS_WHY "no Python interpreter to run tools/hand_model_convert.py")
  else()
    file(SHA256 "${HUM_HANDS_TASK}" HUM_HANDS_TASK_GOT)
    if(NOT HUM_HANDS_TASK_GOT STREQUAL HUM_HANDS_TASK_SHA256)
      message(FATAL_ERROR
        "the bundle at ${HUM_HANDS_TASK_URL} is not the one this tree expects.\n"
        "  expected ${HUM_HANDS_TASK_SHA256}\n"
        "  got      ${HUM_HANDS_TASK_GOT}")
    endif()
    file(ARCHIVE_EXTRACT INPUT "${HUM_HANDS_TASK}" DESTINATION "${HUM_HANDS_WORK}")
    execute_process(
      COMMAND "${HUM_PYTHON}"
              "${CMAKE_CURRENT_SOURCE_DIR}/../tools/hand_model_convert.py"
              "${HUM_HANDS_WORK}/hand_detector.tflite"
              "${HUM_HANDS_WORK}/hand_landmarks_detector.tflite"
              "${HUM_HANDS_WORK}/hands.humnet"
      RESULT_VARIABLE HUM_HANDS_CONV OUTPUT_QUIET ERROR_VARIABLE HUM_HANDS_CONV_ERR)
    if(NOT HUM_HANDS_CONV EQUAL 0)
      set(HUM_HANDS_WHY "the conversion failed (${HUM_HANDS_CONV_ERR})")
    else()
      file(SHA256 "${HUM_HANDS_WORK}/hands.humnet" HUM_HANDS_GOT)
      if(NOT HUM_HANDS_GOT STREQUAL HUM_HANDS_SHA256)
        message(FATAL_ERROR
          "the converted hand model is not the one this tree expects.\n"
          "  expected ${HUM_HANDS_SHA256}\n"
          "  got      ${HUM_HANDS_GOT}")
      endif()
      file(COPY "${HUM_HANDS_WORK}/hands.humnet"
           DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../assets/Models/hands")
      message(STATUS "hand model rebuilt and verified")
    endif()
  endif()

  if(HUM_HANDS_WHY)
    message(WARNING
      "no hand model: ${HUM_HANDS_WHY}.\n"
      "  Hand tracking will do nothing where the system has no tracker of its own.\n"
      "  assets/Models/hands/README.md has the commands that build the file by hand.")
  endif()
  file(REMOVE_RECURSE "${HUM_HANDS_WORK}")
endif()

# ---- organism SDK (public headers + minimal runtime) ----
# The boundary organism packs are compiled against: the Organism base
# contract, Transport/Pattern/Parameter types, header-only DSP helpers, and the
# live WAV writer. Deliberately host-free (no io/ document model, no gui/).
add_library(hum_sdk STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/Transport.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/Organism.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/LiveWavWriter.cpp
)
target_include_directories(hum_sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include)
target_compile_features(hum_sdk PUBLIC cxx_std_17)
target_link_libraries(hum_sdk PUBLIC
  juce::juce_core
  juce::juce_audio_basics
  juce::juce_audio_formats
)
# Shared JUCE configuration: PUBLIC here so the host and every pack compile JUCE
# module code with identical settings. JUCE_STANDALONE_APPLICATION is NOT among
# them: no JUCE module reads it, juce_add_gui_app sets it for the app already,
# and the plugin client defines it as JucePlugin_Build_Standalone - setting it
# here only redefined that, once per plugin translation unit.
target_compile_definitions(hum_sdk PUBLIC
  JUCE_USE_CURL=0
  JUCE_WEB_BROWSER=0
  # Decode common compressed formats in addition to WAV/AIFF. registerBasicFormats()
  # picks these up at compile time. (ALAC needs CoreAudio and is Apple-only.)
  JUCE_USE_FLAC=1
  JUCE_USE_OGGVORBIS=1
  JUCE_USE_MP3AUDIOFORMAT=1
  # Plugin hosting (Sprint 8): formats the host can load.
  JUCE_PLUGINHOST_VST3=1
  JUCE_PLUGINHOST_LV2=1
  # CamIn: juce_video's CameraDevice is compiled out unless this is set.
  JUCE_USE_CAMERA=1
  # A GUI exception logs and the session survives, instead of abort mid-set.
  JUCE_CATCH_UNHANDLED_EXCEPTIONS=1
)
if(APPLE)
  # AudioUnits exist only on macOS - and legacy ancestor patches identify
  # their AU plugins by packed component codes we can match (see PluginHost).
  target_compile_definitions(hum_sdk PUBLIC JUCE_PLUGINHOST_AU=1)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # JACK - the Linux pro-audio door, and how PipeWire serves low latency.
  # Needs <jack/jack.h> (libjack-jackd2-dev) to COMPILE, but JUCE dlopens
  # libjack at runtime, so a build without it still runs and offers ALSA.
  # Optional, so a dev box without the headers still builds.
  find_path(HUM_JACK_INCLUDE_DIR jack/jack.h)
  if(HUM_JACK_INCLUDE_DIR)
    target_compile_definitions(hum_sdk PUBLIC JUCE_JACK=1)
  else()
    message(STATUS "jack/jack.h not found - no JACK backend "
                   "(install libjack-jackd2-dev and re-configure to get one)")
  endif()
endif()
if(WIN32)
  # ASIO - the Windows low-latency door. Steinberg dual-licensed the SDK under
  # GPLv3 on 2025-10-15 and AGPLv3 §13 permits combining a covered work with
  # GPLv3 code, so the three headers JUCE needs are vendored and ASIO ships by
  # default. HUM_ENABLE_ASIO=OFF builds with no GPLv3 component at all;
  # HUM_ASIO_SDK_DIR points at your own SDK instead. packaging/README.md § ASIO.
  option(HUM_ENABLE_ASIO "Build the ASIO backend (vendored GPLv3 SDK)" ON)
  if(HUM_ENABLE_ASIO)
    if(NOT HUM_ASIO_SDK_DIR AND DEFINED ENV{HUM_ASIO_SDK_DIR})
      set(HUM_ASIO_SDK_DIR "$ENV{HUM_ASIO_SDK_DIR}" CACHE PATH "Steinberg ASIO SDK root")
    endif()
    # NO_DEFAULT_PATH keeps this to the copy we chose: a stray SDK elsewhere on
    # the machine must never decide what a release build links against.
    find_path(HUM_ASIO_INCLUDE_DIR iasiodrv.h
      HINTS "${HUM_ASIO_SDK_DIR}" "${HUM_ASIO_SDK_DIR}/common"
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio_sdk" NO_DEFAULT_PATH)
    if(HUM_ASIO_INCLUDE_DIR)
      target_compile_definitions(hum_sdk PUBLIC JUCE_ASIO=1)
      target_include_directories(hum_sdk PUBLIC "${HUM_ASIO_INCLUDE_DIR}")
      message(STATUS "ASIO backend enabled - ${HUM_ASIO_INCLUDE_DIR}")
    else()
      message(FATAL_ERROR "HUM_ENABLE_ASIO is ON but iasiodrv.h was not found. "
        "engine/third_party/asio_sdk should hold it - re-clone, point "
        "HUM_ASIO_SDK_DIR at an SDK copy, or configure with -DHUM_ENABLE_ASIO=OFF.")
    endif()
  else()
    message(STATUS "ASIO backend disabled - WASAPI exclusive remains available.")
  endif()
endif()

# ---- vendored FM engines (the pH organism's voices) ----
# msfa: the six-operator FM engine from music-synthesizer-for-android
# (Apache-2.0). nuked_opn2: the cycle-accurate OPN2 core (LGPL-2.1).
# Defined before the packs so pack targets can link them.
add_library(msfa STATIC
  third_party/msfa/sin.cc
  third_party/msfa/exp2.cc
  third_party/msfa/freqlut.cc
  third_party/msfa/env.cc
  third_party/msfa/pitchenv.cc
  third_party/msfa/lfo.cc
  third_party/msfa/fm_core.cc
  third_party/msfa/fm_op_kernel.cc
  third_party/msfa/patch.cc
  third_party/msfa/dx7note.cc)
target_include_directories(msfa PUBLIC third_party/msfa)
add_library(nuked_opn2 STATIC third_party/nuked-opn2/ym3438.c)
target_include_directories(nuked_opn2 PUBLIC third_party/nuked-opn2)
# Vendored code compiles QUIET. It is upstream's, kept verbatim so it can be
# re-synced, and its warnings are not ours to fix in place - one of them (an
# implicit `using namespace std`) is the only thing left in an otherwise clean
# build, and a build with one permanent warning trains you to ignore the next.
if(NOT MSVC)
  target_compile_options(msfa PRIVATE -w)
  target_compile_options(nuked_opn2 PRIVATE -w)
endif()

# ---- organism packs ----
# Pre-installed (statically linked into the host): core (I/O + routing
# plumbing) and humus (the originals). An ADDON pack is still built here, but
# the host only meets it as an installed .humpack; the dev binaries keep it
# statically linked so repo patches render and the suites stay hermetic.
# Packs are discovered, not listed. A pack is a directory holding a CMakeLists
# that declares HUM_PACK_ROLE and builds hum_pack_<directory>, so nothing here
# names one and a tree that arrives without an add-on builds without it.
set(HUM_BUILTIN_PACKS "")
set(HUM_ADDON_PACKS "")
file(GLOB HUM_PACK_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../packs/*)
list(SORT HUM_PACK_DIRS)
foreach(pack_dir ${HUM_PACK_DIRS})
  if(NOT EXISTS ${pack_dir}/CMakeLists.txt)
    continue()
  endif()
  get_filename_component(pack_name ${pack_dir} NAME)
  set(HUM_PACK_ROLE "")
  add_subdirectory(${pack_dir} pack_${pack_name})
  if("${HUM_PACK_ROLE}" STREQUAL "builtin")
    list(APPEND HUM_BUILTIN_PACKS ${pack_name})
  elseif("${HUM_PACK_ROLE}" STREQUAL "addon")
    list(APPEND HUM_ADDON_PACKS ${pack_name})
  endif()
endforeach()

# ---- vendored Monocypher (Ed25519 license verification) ----
# Committed at third_party/monocypher (4.0.2, CC0/BSD-2). The OPTIONAL
# ed25519 files carry the RFC 8032 crypto_ed25519_check the hub licenses
# need - the core lib's crypto_eddsa_* is BLAKE2b-EdDSA and incompatible.
add_library(monocypher STATIC
  third_party/monocypher/monocypher.c
  third_party/monocypher/monocypher-ed25519.c)
target_include_directories(monocypher PUBLIC third_party/monocypher)

# ---- Ableton Link (network tempo/beat/phase sync) ----
# Vendored here, fetched by the source distribution, which .gitattributes
# strips it from. Pinned to the Link-4.0 tag's commit and to the
# asio-standalone submodule that tag carries. SOURCE_SUBDIR names a directory
# with no CMakeLists.txt, which populates the source without add_subdirectory,
# so Link's tests and examples stay out. The two layouts differ, so the include
# dirs are chosen with the tree. Offline:
# -DFETCHCONTENT_SOURCE_DIR_ABLETONLINK=<path>.
#
# Header-only. Only gui/LinkSync.cpp may include Link.hpp - it drags in Asio.
set(HUM_LINK_VENDORED "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ableton_link")
if(EXISTS "${HUM_LINK_VENDORED}/include/ableton/Link.hpp")
  set(HUM_LINK_INCLUDES "${HUM_LINK_VENDORED}/include"
                        "${HUM_LINK_VENDORED}/asio/include")
else()
  message(STATUS "Ableton Link is not in the tree - fetching it")
  FetchContent_Declare(
    AbletonLink
    GIT_REPOSITORY https://github.com/Ableton/link.git
    GIT_TAG        e9a2e414d63f55f1aad158370b007a6fbdc1eeb9
    GIT_SUBMODULES modules/asio-standalone
    SOURCE_SUBDIR  cmake_include
  )
  FetchContent_MakeAvailable(AbletonLink)
  set(HUM_LINK_INCLUDES "${abletonlink_SOURCE_DIR}/include"
                        "${abletonlink_SOURCE_DIR}/modules/asio-standalone/asio/include")
endif()

add_library(ableton_link INTERFACE)
# SYSTEM, so upstream's warnings stay upstream's: its payload keys are
# multi-character constants that GCC and Clang warn on by default.
target_include_directories(ableton_link SYSTEM INTERFACE ${HUM_LINK_INCLUDES})

if(UNIX)
  target_compile_definitions(ableton_link INTERFACE LINK_PLATFORM_UNIX=1)
endif()
if(APPLE)
  target_compile_definitions(ableton_link INTERFACE LINK_PLATFORM_MACOSX=1)
elseif(WIN32)
  target_compile_definitions(ableton_link INTERFACE LINK_PLATFORM_WINDOWS=1)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux|kFreeBSD|GNU")
  target_compile_definitions(ableton_link INTERFACE LINK_PLATFORM_LINUX=1)
  target_link_libraries(ableton_link INTERFACE atomic pthread)
endif()

# ---- core engine library (graph, document model, host services) ----
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
# Dev fallback for the staged packs dir (manifests, editor layouts, help). All
# targets inherit it; shipped builds resolve <exeDir>/packs first.
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
  # juce_audio_processors' AU hosting references AUGenericView; the JUCE GUI
  # targets pull CoreAudioKit in themselves, but plain executables linking
  # hum_core (the CLI, hum_tests) need it spelled out.
  target_link_libraries(hum_core PUBLIC "-framework CoreAudioKit")
  # Game controllers (GamepadHostMac.mm) - PS/Xbox/MFi pads over USB/Bluetooth.
  target_link_libraries(hum_core PUBLIC "-framework GameController")
endif()

# ---- CLI app ----
add_executable(hum src/app/main.cpp)
target_include_directories(hum PRIVATE ${HUM_GENERATED_DIR})
add_dependencies(hum hum_build_id)
# Dev binary: statically carries any addon pack so repo patches render without
# installing the .humpack. The GUI app does not.
target_link_libraries(hum PRIVATE hum_core)
# The CLI carries whatever add-ons this tree has; which ones is generated, so
# no source file names a pack.
set(HUM_ADDON_DECLS "")
set(HUM_ADDON_CALLS "")
set(HUM_ADDON_IDS "")
foreach(addon ${HUM_ADDON_PACKS})
  target_link_libraries(hum PRIVATE hum_pack_${addon})
  string(APPEND HUM_ADDON_DECLS "void hum_register_pack_${addon}(Registry&);\n")
  string(APPEND HUM_ADDON_CALLS "    hum_register_pack_${addon}(Registry::instance());\n")
  list(APPEND HUM_ADDON_IDS "\"${addon}\"")
endforeach()
list(JOIN HUM_ADDON_IDS ", " HUM_ADDON_IDS)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddonPacks.h.in
               ${HUM_GENERATED_DIR}/hum_addon_packs.h @ONLY)
if(HUM_ADDON_PACKS)
  target_compile_definitions(hum PRIVATE HUM_STATIC_ADDON_PACKS=1)
endif()
if(HUM_LIVE_AUDIO)
  target_link_libraries(hum PRIVATE juce::juce_audio_devices)
  target_compile_definitions(hum PRIVATE HUM_LIVE_AUDIO=1)
endif()

# ---- tests (JUCE built-in UnitTest runner) ----
# The suite is our regression battery rather than part of the shipped work, and
# its organism cases instantiate the addon pack directly - so the source
# distribution arrives without either and builds no tests.
set(HUM_TESTS OFF)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.cpp)
  set(HUM_TESTS ON)
endif()

# One file per test category; each registers itself via a static instance.
# Conditional categories are gated by in-file #ifdef, so all are listed
# unconditionally here (an absent define makes the file compile to nothing).
if(HUM_TESTS)
add_executable(hum_tests tests/run_tests.cpp)
# A pack's own suite lives with it, so it arrives and leaves with the pack and
# nothing here has to know which packs exist.
file(GLOB HUM_PACK_TESTS CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/../packs/*/tests/*.cpp)
target_sources(hum_tests PRIVATE ${HUM_PACK_TESTS})
target_sources(hum_tests PRIVATE
  tests/OrganismTests.cpp
  tests/RecentFilesTests.cpp
  tests/CategoryTests.cpp
  tests/LayoutProviderTests.cpp
  tests/GraphTests.cpp
  tests/BypassTests.cpp
  tests/DelayTests.cpp
  tests/LoaderTests.cpp
  tests/Sf2Tests.cpp
  tests/GuiEnginePathTests.cpp
  tests/AudioTrackTests.cpp
  tests/CrossoverTests.cpp
  tests/DynamicsTests.cpp
  tests/HelixTests.cpp
  tests/AutomationTests.cpp
  tests/ClipOpsTests.cpp
  tests/RecordSessionTests.cpp
  tests/TracksLayoutTests.cpp
  tests/WaveformCacheTests.cpp
  tests/DeckTests.cpp
  tests/SamplerTests.cpp
  tests/LeafcutterTests.cpp
  tests/SoundSpaceTests.cpp
  tests/DspCoreTests.cpp
  tests/ControlShapeTests.cpp
  tests/FormulaTests.cpp
  tests/NumberFormatTests.cpp
  tests/PrimitivesTests.cpp
  tests/MorseTests.cpp
  tests/DiodeLadderTests.cpp
  tests/DropsTests.cpp
  tests/TapTempoTests.cpp
  tests/TribeTests.cpp
  tests/SynthTests.cpp
  tests/SideChainTests.cpp
  tests/SpectralTests.cpp
  tests/PitchTests.cpp
  tests/TuningTests.cpp
  tests/ScaleTests.cpp
  tests/ScaleLibraryTests.cpp
  tests/SwingTests.cpp
  tests/MtsTuningTests.cpp
  tests/BendRetunerTests.cpp
  tests/PianoteqLiveTests.cpp
  tests/RhizomeTests.cpp
  tests/TuningNodeTests.cpp
  tests/StereoToolTests.cpp
  tests/NoteScheduleTests.cpp
  tests/PhonoTests.cpp
  tests/WaveTests.cpp
  tests/PrismTests.cpp
  tests/ChordTests.cpp
  tests/DnaTests.cpp
  tests/RollPlotTests.cpp
  tests/GenieTests.cpp
  tests/RandomizeTests.cpp
  tests/LayoutSpecTests.cpp
  tests/ToolbarLadderTests.cpp
  tests/PodModelTests.cpp
  tests/PodMidiTests.cpp
  tests/PackUpdatesTests.cpp
  tests/ParamHistoryTests.cpp
  tests/PresetLibraryTests.cpp
  src/gui/LayoutLoader.cpp
  tests/CameraInTests.cpp
  tests/AssistantTests.cpp
  tests/AutomixPlanTests.cpp
  tests/SpectrumBandsTests.cpp
  tests/RecipeSynthTests.cpp
  tests/OllamaWireTests.cpp
  tests/PackSdkTests.cpp
  tests/PluginHostTests.cpp
  tests/PluginListStoreTests.cpp
  tests/LooperFlowTests.cpp
  tests/MetapadTests.cpp
  tests/MidiBusTests.cpp
  tests/ParamGroupingTests.cpp
  tests/PickerModelTests.cpp
  tests/WizardFlowTests.cpp
  tests/GuideFlowTests.cpp
  tests/HandsTests.cpp
  tests/SigilTests.cpp
  tests/MidiGraphTests.cpp
  tests/MidiMonitorTests.cpp
  tests/MidiSyncTests.cpp
  tests/LinkChaseTests.cpp
  tests/LinkVendorTests.cpp
  tests/RtWordTests.cpp
  tests/GraphLayoutTests.cpp
  tests/AutoWireTests.cpp
  tests/OscTests.cpp
  tests/ModControlTests.cpp
  tests/VisualUniformsTests.cpp
  tests/IsfParseTests.cpp
  tests/SsfShimTests.cpp
  tests/MixRecorderTests.cpp
  tests/HumusMacroTests.cpp
  tests/HumusPodTests.cpp
  tests/SerialTests.cpp
  tests/PhTests.cpp
  tests/LfoScopeTests.cpp
  tests/LatencyTests.cpp
  tests/PerfBoxTests.cpp
  tests/AutosaveTests.cpp
  tests/UiWatchdogTests.cpp
  tests/BridgeRingTests.cpp
  tests/BridgeTests.cpp
  tests/LicenseCheckTests.cpp
  tests/HubProtocolTests.cpp
  tests/TelemetrySpoolTests.cpp
  tests/HelpTests.cpp
  tests/ConsoleTests.cpp
  tests/MixerTests.cpp
  tests/DocumentSetTests.cpp)
# Tests statically carry the addon packs too - the organism suites instantiate
# their classes directly and must stay hermetic (no dlopen).
# The tour's copy is compiled in, and GuideFlowTests reads it.
target_link_libraries(hum_tests PRIVATE hum_core hum_assets)
foreach(addon ${HUM_ADDON_PACKS})
  target_link_libraries(hum_tests PRIVATE hum_pack_${addon})
  target_compile_definitions(hum_tests PRIVATE HUM_STATIC_ADDON_PACKS=1)
endforeach()
target_compile_definitions(hum_tests PRIVATE
  HUM_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/..")
if(HUM_DYN_PACKS)
  # SDK conformance dogfood: the test dlopens the dynamically-built packs.
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

# ---- plugin-hosting test fixture (a minimal in-tree VST3) ----
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
    COPY_PLUGIN_AFTER_BUILD FALSE)
  target_sources(hum_testgain PRIVATE tests/fixtures/testgain/TestGain.cpp)
  target_link_libraries(hum_testgain PRIVATE juce::juce_audio_utils)
  target_compile_definitions(hum_testgain PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  # The scan dir is the folder CONTAINING TestGain.vst3 (three levels above the
  # bundle-internal binary dir).
  target_compile_definitions(hum_tests PRIVATE
    HUM_TESTGAIN_VST3_DIR="$<TARGET_FILE_DIR:hum_testgain_VST3>/../../..")
  add_dependencies(hum_tests hum_testgain_VST3)
endif()

# ---- the Humus plugin: the whole engine as a DAW plugin ----
option(HUM_PLUGIN_EXPORT "Build the Humus plugin (VST3; +AU on macOS)" ON)
if(HUM_PLUGIN_EXPORT)
  if(APPLE)
    set(HUM_HUMUS_FORMATS VST3 AU)
  else()
    set(HUM_HUMUS_FORMATS VST3)
  endif()
  juce_add_plugin(Humus
    PRODUCT_NAME "Humus"
    COMPANY_NAME "Phobos Instruments"
    COMPANY_WEBSITE "https://humus.phobos-instruments.com"
    # Distinct from the app's, and spelled out: derived from the company name it
    # would contain spaces. Hosts identify the plugin by the codes below, not this.
    BUNDLE_ID "com.phobos-instruments.humus-plugin"
    PLUGIN_MANUFACTURER_CODE Phob
    PLUGIN_CODE Humu
    FORMATS ${HUM_HUMUS_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  target_sources(Humus PRIVATE src/plugin/HumusProcessor.cpp)
  target_include_directories(Humus PRIVATE src ${HUM_GENERATED_DIR})
  add_dependencies(Humus hum_build_id)
  target_link_libraries(Humus PRIVATE hum_core juce::juce_audio_utils)
  target_compile_definitions(Humus PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  # Stage the pre-installed organism packs (manifests/schemas) where the plugin
  # bundle's packsRootDir() looks - the builtin packs; an addon is
  # not builtin. Every format gets its own copy: a DAW loading the .component
  # resolves packs beside ITS binary, not the VST3's.
  if(APPLE)
    set(HUM_PLUGIN_TS "--timestamp")
    if(HUM_CODESIGN_IDENTITY STREQUAL "-")
      set(HUM_PLUGIN_TS "--timestamp=none")
    endif()
  endif()
  function(hum_stage_plugin_packs target)
    if(APPLE)
      # Packs are DATA. Under Contents/MacOS they are code as far as codesign is
      # concerned, and it refuses to seal a bundle holding files it cannot sign
      # ("code object is not signed at all - In subcomponent: .../Lumen.h").
      set(dest "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/packs")
      set(adest "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/assets")
    else()
      set(dest "$<TARGET_FILE_DIR:${target}>/packs")
      set(adest "$<TARGET_FILE_DIR:${target}>/assets")
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
      # Clear first: copy_directory never deletes, so a file removed from a pack
      # (or moved between packs, like Matrix) would linger. The second path is
      # where these used to land, and clears it out of an older build tree.
      COMMAND ${CMAKE_COMMAND} -E rm -rf "${dest}" "$<TARGET_FILE_DIR:${target}>/packs"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../packs/core" "${dest}/core"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../packs/humus" "${dest}/humus"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av" "${dest}/av"
      # copy_directory takes the sources too, and a loaded plugin has no use for
      # them - that code is already linked in. Stripped by extension so the
      # manifests and editor blueprints beside them survive.
      COMMAND ${CMAKE_COMMAND} -DDIR=${dest}
              -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_pack_sources.cmake"
      # Everything a patch can name, so asset:Shaders/breathe.frag resolves
      # inside a host. Not patches: a plugin has no "Load a demo".
      COMMAND ${CMAKE_COMMAND} -E rm -rf "${adest}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets" "${adest}"
      COMMAND ${CMAKE_COMMAND} -E rm -rf "${adest}/patches"
      COMMAND ${CMAKE_COMMAND} -DDIR=${adest}
              -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_junk.cmake"
      COMMENT "Staging organism packs and assets into ${target}")
    if(APPLE)
      # This runs after JUCE has sealed the bundle, so the seal no longer covers
      # Resources. Re-sign, or the plugin arrives with a broken signature.
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND codesign --force --options runtime ${HUM_PLUGIN_TS}
                --sign "${HUM_CODESIGN_IDENTITY}" "$<TARGET_BUNDLE_DIR:${target}>"
        COMMENT "Sealing ${target} (identity: ${HUM_CODESIGN_IDENTITY})")
    endif()
  endfunction()
  hum_stage_plugin_packs(Humus_VST3)
  if(APPLE)
    hum_stage_plugin_packs(Humus_AU)
  endif()
  if(HUM_HOSTING_TESTS AND HUM_TESTS)
    target_compile_definitions(hum_tests PRIVATE
      HUM_HUMUS_VST3_DIR="$<TARGET_FILE_DIR:Humus_VST3>/../../.."
      HUM_HAVE_HUMUS_PLUGIN=1)
    # The processor class is tested directly too (the .vst3 covers the wrapper).
    target_sources(hum_tests PRIVATE src/plugin/HumusProcessor.cpp)
    add_dependencies(hum_tests Humus_VST3)
  endif()
endif()

enable_testing()
if(HUM_TESTS)
  add_test(NAME hum_tests COMMAND hum_tests)
endif()

# ---- GUI app (JUCE desktop) ----
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

# macOS-only Objective-C++ sources - compiling these on Linux fails (no cc1objplus).
if(APPLE)
  list(APPEND HUM_GUI_SOURCES
    src/gui/EmbeddedPluginViewMac.mm
    src/gui/PluginEditorWindowMac.mm
    src/gui/MacCursors.mm
    src/gui/GamepadHostMac.mm
    src/gui/VideoLayerMac.mm
    src/gui/AppNapMac.mm
    src/gui/FileDragImageMac.mm
  )
endif()

# Everything compiled into the binary rather than staged beside it: the logo,
# the welcome tour's copy, and any per-glyph icon override. Nothing here can go
# missing at runtime or be edited by a user, which is the point. Outside the
# HUM_GUI guard because hum_tests reads the tour copy too.
file(GLOB HUM_EMBEDDED_ART CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/glyphs/*.svg"
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/glyphs/*.png")
file(GLOB HUM_EMBEDDED_GUIDE CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/guide/*.md")
juce_add_binary_data(hum_assets SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.png"
  ${HUM_EMBEDDED_ART}
  ${HUM_EMBEDDED_GUIDE})

option(HUM_GUI "Build the JUCE GUI app" ON)
if(HUM_GUI)
  juce_add_gui_app(hum_gui PRODUCT_NAME "Humus"
    # Without a company JUCE stamps the app "yourcompany"; without an explicit
    # id it derives one containing spaces, which is not a valid bundle id.
    COMPANY_NAME "Phobos Instruments"
    COMPANY_WEBSITE "https://humus.phobos-instruments.com"
    BUNDLE_ID "com.phobos-instruments.humus"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/icon-1024.png"
    # Notarization requires the hardened runtime - and the hardened runtime
    # enables LIBRARY VALIDATION, which refuses to dlopen any binary not signed
    # by our own Team ID. That would break the two things this app is built on:
    # organism packs (.humpack, third parties included) and hosted VST3/AU
    # plugins. disable-library-validation is Apple's supported answer for
    # exactly this - it is what every plugin host declares - and notarization
    # still passes with it.
    HARDENED_RUNTIME_ENABLED TRUE
    # device.camera / device.audio-input: under the hardened runtime the usage
    # strings below are NOT enough - without the matching entitlement macOS
    # never even shows the permission prompt, it just hands the app an empty
    # camera list and a dead input. (That was "the app doesn't show cameras":
    # every hum_gui build is hardened-signed below, so the app was blind while
    # the unhardened snapshot/CLI tools listed cameras fine.)
    HARDENED_RUNTIME_OPTIONS "com.apple.security.cs.disable-library-validation"
                             "com.apple.security.device.camera"
                             "com.apple.security.device.audio-input"
                             "com.apple.security.device.bluetooth"
    # CamIn (camera -> control signals): macOS prompts with this text the
    # first time a patch enables a camera organism.
    CAMERA_PERMISSION_ENABLED TRUE
    CAMERA_PERMISSION_TEXT "Humus uses the camera for CamIn organisms - motion and pose become patchable control signals. Video never leaves the app."
    # SoundIn (live input) and the recorders read the audio interface - macOS
    # requires this usage string or it silently hands the app a dead input.
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Humus uses your audio input so SoundIn and the recorders can capture a microphone, instrument or line signal. Audio stays on your machine."
    # Settings > MIDI > Bluetooth MIDI devices opens the system pairing window,
    # which scans for BLE peripherals. macOS terminates a process that reaches
    # CoreBluetooth without this string - a kill, not an exception, so it looks
    # like the app crashed on the button.
    BLUETOOTH_PERMISSION_ENABLED TRUE
    BLUETOOTH_PERMISSION_TEXT "Humus uses Bluetooth to find and pair wireless MIDI controllers and keyboards. Nothing is sent anywhere."
    # The AI provider endpoint is user-configured and commonly plain HTTP on
    # the LAN (Ollama): without the ATS exception NSURLSession refuses http://
    # outright, and macOS 15 needs the local-network usage text for its
    # consent prompt when the app first touches a LAN address.
    # The .hum document type. LaunchServices needs BOTH halves: the UTI export
    # declares what a .hum IS, CFBundleDocumentTypes declares that we open it.
    # Without them a double-clicked patch never reaches the app, and there is no
    # type for the first-run "make Humus the default" prompt to claim.
    # A patch is XML, so the UTI conforms to public.xml - a .hum stays inspectable
    # by anything that reads XML, which is the whole point of one vocabulary.
    #
    # Only .hum is declared. We used to import the ancestor's type as well, to
    # register as an Alternate handler for .amh; the Windows installer claims
    # .hum and nothing else, and three platforms disagreeing about which files
    # this app owns is worse than not offering the association at all. File >
    # Open still reads .amh, and always will - that is a reader, not a claim.
    PLIST_TO_MERGE [[<plist version="1.0"><dict>
      <key>NSAppTransportSecurity</key>
      <dict><key>NSAllowsArbitraryLoads</key><true/></dict>
      <key>NSLocalNetworkUsageDescription</key>
      <string>Humus connects to your AI server on the local network (Settings → AI) for the assistant, preset and sample features.</string>
      <key>UTExportedTypeDeclarations</key>
      <array><dict>
        <key>UTTypeIdentifier</key><string>com.totel.humus.patch</string>
        <key>UTTypeDescription</key><string>Humus Patch</string>
        <key>UTTypeConformsTo</key><array><string>public.xml</string></array>
        <key>UTTypeTagSpecification</key>
        <dict><key>public.filename-extension</key><array><string>hum</string></array></dict>
      </dict></array>
      <key>CFBundleDocumentTypes</key>
      <array>
        <dict>
          <key>CFBundleTypeName</key><string>Humus Patch</string>
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
    # project(Humus VERSION ...) above is the ONE version source: this macro,
    # Main.cpp, the Makefile's dist names and the installer all read it.
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
  )

  # Ship the PRE-INSTALLED organism packs (manifests, editor layouts, help)
  # next to the binary so the app is self-contained: core + humus only - the
  # an addon arrives via .humpack install. The staged copies are
  # cleared first: copy_directory never deletes, so anything removed from a
  # pack (or moved between packs) would
  # otherwise linger beside the binary forever and ghost in the palette.
  # On macOS the packs are DATA and must land in Contents/Resources: codesign
  # treats everything under Contents/MacOS as code, so staging them beside the
  # binary made it refuse to seal the bundle ("code object is not signed at
  # all - In subcomponent: .../packs/core/organisms/Bus/Bus.h"). Elsewhere,
  # beside the binary is right. packsRootDir() resolves both.
  if(APPLE)
    set(HUM_GUI_PACKS_DIR "$<TARGET_BUNDLE_CONTENT_DIR:hum_gui>/Resources/packs")
  else()
    set(HUM_GUI_PACKS_DIR "$<TARGET_FILE_DIR:hum_gui>/packs")
  endif()
  # NOT a POST_BUILD on hum_gui: those run only when the EXECUTABLE relinks,
  # so a manifest/blueprint-only edit shipped a stale bundle (the .fsh scene
  # filter sat in the repo while the installed app kept the old picker). The
  # copy below is a stamped custom command that depends on the pack files
  # themselves; the signing has to ride with it because signing seals the
  # bundle contents.
  file(GLOB_RECURSE HUM_BUNDLED_PACK_FILES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/../packs/core/*"
       "${CMAKE_CURRENT_SOURCE_DIR}/../packs/humus/*"
       "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av/*")
  file(GLOB_RECURSE HUM_BUNDLED_ASSET_FILES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/../assets/*")

  # assets/ ships whole and keeps its folder names, so the tree in the bundle,
  # the tree in the repo and the one in Documents/Humus are the same shape
  # (hum::bundledAssetsDir, hum::userAssetsDir).
  if(APPLE)
    set(HUM_GUI_ASSETS_DIR "$<TARGET_BUNDLE_CONTENT_DIR:hum_gui>/Resources/assets")
  else()
    set(HUM_GUI_ASSETS_DIR "$<TARGET_FILE_DIR:hum_gui>/assets")
  endif()
  # A dev tree keeps the user's own jams (my-*.hum) beside the shipped patches;
  # copy_directory takes everything, so they are removed again afterwards.
  file(GLOB HUM_PRIVATE_PATCHES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/../assets/patches/my-*")
  set(HUM_PRIVATE_STRIP_CMD "")
  foreach(f ${HUM_PRIVATE_PATCHES})
    get_filename_component(n "${f}" NAME)
    list(APPEND HUM_PRIVATE_STRIP_CMD
         COMMAND ${CMAKE_COMMAND} -E rm -f "${HUM_GUI_ASSETS_DIR}/patches/${n}")
  endforeach()

  # Sign the .app - and it has to be done here by hand. juce_add_gui_app's
  # HARDENED_RUNTIME_ENABLED only writes the .entitlements file and then wires
  # it through XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS, which the Xcode generator
  # reads and Ninja ignores completely. So on this build the flag generates a
  # file nobody applies: the app would ship WITHOUT the hardened runtime, and
  # you would not find out until notarization rejected it.
  #
  # Must run after the pack staging above - signing seals the bundle's contents,
  # so anything copied in afterwards invalidates the signature.
  set(HUM_GUI_SIGN_CMD "")
  if(APPLE)
    set(HUM_GUI_ENTITLEMENTS
        "${CMAKE_CURRENT_BINARY_DIR}/hum_gui_artefacts/JuceLibraryCode/hum_gui.entitlements")
    # A secure timestamp needs a real identity and the network; ad-hoc dev
    # builds must opt out or codesign fails.
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

  # One stamped step: stage packs + assets into the bundle, then seal it.
  # Re-runs whenever the executable OR any bundled file changes.
  set(HUM_GUI_BUNDLE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/hum_gui_bundle.stamp")
  add_custom_command(OUTPUT "${HUM_GUI_BUNDLE_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${HUM_GUI_PACKS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../packs/core" "${HUM_GUI_PACKS_DIR}/core"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../packs/humus" "${HUM_GUI_PACKS_DIR}/humus"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av" "${HUM_GUI_PACKS_DIR}/av"
    # copy_directory takes the whole folder, sources included - a shipped app
    # has no use for pack .cpp/.h (that code is already linked in), and they are
    # what codesign chokes on. Strip by EXTENSION: organisms/ also holds the
    # manifests and editor blueprints, so deleting the folder would take the
    # schemas with it and the app would come up with no parameters at all.
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
            # The signature carries the entitlements: an edited entitlements
            # file (new hardened-runtime option) must re-seal the bundle, or
            # the app keeps shipping with yesterday's permissions.
            ${HUM_GUI_ENTITLEMENTS}
    COMMENT "Bundling packs + assets and sealing Humus.app (identity: ${HUM_CODESIGN_IDENTITY})")
  add_custom_target(hum_gui_bundle ALL DEPENDS "${HUM_GUI_BUNDLE_STAMP}")

  # Offscreen snapshot tool (renders the GUI to a PNG headlessly). Ours: it
  # drives the check batteries and shares their fixtures, so it is built only
  # where those exist.
  if(HUM_TESTS)
  juce_add_console_app(hum_snapshot)
  target_sources(hum_snapshot PRIVATE
    src/gui/Snapshot.cpp
    src/gui/snapshot/BoardsChrome.cpp
    src/gui/snapshot/BoardsPanes.cpp
    src/gui/snapshot/BoardsScenes.cpp
    src/gui/snapshot/ChecksAutomation.cpp
    src/gui/snapshot/ChecksBox.cpp
    src/gui/snapshot/ChecksEdit.cpp
    src/gui/snapshot/ChecksGraph.cpp
    src/gui/snapshot/ChecksHelp.cpp
    src/gui/snapshot/ChecksLayout.cpp
    src/gui/snapshot/ChecksClip.cpp
    src/gui/snapshot/ChecksHelix.cpp
    src/gui/snapshot/ChecksRoll.cpp
    src/gui/snapshot/ChecksSelect.cpp
    src/gui/snapshot/ChecksSession.cpp
    src/gui/snapshot/ChecksShader.cpp
    src/gui/snapshot/ChecksUnits.cpp
    src/gui/snapshot/ExportOrganisms.cpp
    src/gui/snapshot/HostDiagnostics.cpp
    ${HUM_GUI_SOURCES}
  )
  # The doc exporter stages real data into a few concrete organisms (Hands
  # test signals, CameraIn preview frames), so it sees the pack headers too.
  # tests/ for the shared fixtures: the snapshot checks are the other half of
  # this project's test suite, and a bank fixture written twice is a bank
  # fixture that drifts.
  target_include_directories(hum_snapshot PRIVATE src ${HUM_GENERATED_DIR}
                             ../packs/humus/organisms tests)
  add_dependencies(hum_snapshot hum_build_id)
  target_link_libraries(hum_snapshot PRIVATE
    hum_core
    hum_assets
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_devices
    juce::juce_audio_utils
    juce::juce_video        # camera names for "video-inputs" combos
    juce::juce_cryptography # links only - the boards never download
    juce::juce_opengl       # links only - headless render never opens a VisualWindow
  )
  if(APPLE)
    target_link_libraries(hum_snapshot PRIVATE
      "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")
  endif()
  if(HUM_DYN_PACKS)
    # The style boards must show the REAL editors, an addon pack's included.
    # The snapshot does not LINK it (it mirrors the GUI, which meets an addon
    # only as an installed pack) - it installs the .humpack this build
    # produced, into a temp dir, through the ordinary pack path. So the boards
    # stay hermetic to the repo AND exercise the same ABI a user's install does.
    target_compile_definitions(hum_snapshot PRIVATE
      HUM_HUMPACKS_DIR="${CMAKE_BINARY_DIR}/humpacks")
    foreach(addon ${HUM_ADDON_PACKS})
      add_dependencies(hum_snapshot humpack_${addon})
    endforeach()
  endif()

  # X11 composite mirror (Linux): plugin UIs rendered through our window.
  # JUCE dlopens X11 itself, so the mirror brings its own link deps.
  if(UNIX AND NOT APPLE)
    find_package(X11 COMPONENTS Xcomposite Xext)
    if(X11_FOUND AND X11_Xcomposite_FOUND)
      foreach(tgt hum_gui hum_snapshot)
        target_link_libraries(${tgt} PRIVATE X11::X11 X11::Xcomposite X11::Xext)
        target_compile_definitions(${tgt} PRIVATE HUM_X11_MIRROR=1)
      endforeach()
    endif()
  endif()

  target_compile_definitions(hum_snapshot PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

  # Stage the assets beside the check binary too. Without this every asset check
  # reads the repo copy through the HUM_PACKS_DIR fallback and nothing ever
  # exercises the layout a user actually gets.
  add_custom_command(TARGET hum_snapshot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:hum_snapshot>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/../assets" "$<TARGET_FILE_DIR:hum_snapshot>/assets"
    COMMAND ${CMAKE_COMMAND} -DDIR=$<TARGET_FILE_DIR:hum_snapshot>/assets
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../tools/strip_junk.cmake"
    COMMENT "Staging assets beside hum_snapshot")
  endif()   # HUM_TESTS
endif()
