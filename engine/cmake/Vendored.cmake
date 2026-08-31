# The pH organism's voices; before pack discovery so packs can link them.
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
add_library(nuked_opm STATIC third_party/nuked-opm/opm.c)
target_include_directories(nuked_opm PUBLIC third_party/nuked-opm)
add_library(nuked_opl3 STATIC third_party/nuked-opl3/opl3.c)
target_include_directories(nuked_opl3 PUBLIC third_party/nuked-opl3)
add_library(resid STATIC
  third_party/resid/sid.cc
  third_party/resid/voice.cc
  third_party/resid/wave.cc
  third_party/resid/envelope.cc
  third_party/resid/filter.cc
  third_party/resid/extfilt.cc
  third_party/resid/pot.cc)
target_include_directories(resid PUBLIC third_party/resid)
# Upstream's warnings stay upstream's; kept verbatim so it can be re-synced.
if(NOT MSVC)
  target_compile_options(msfa PRIVATE -w)
  target_compile_options(nuked_opn2 PRIVATE -w)
  target_compile_options(nuked_opm PRIVATE -w)
  target_compile_options(nuked_opl3 PRIVATE -w)
  target_compile_options(resid PRIVATE -w)
endif()

# The OPTIONAL ed25519 files are the point: the core lib's EdDSA is the
# wrong flavour for the hub licenses (docs/dev/build.md).
add_library(monocypher STATIC
  third_party/monocypher/monocypher.c
  third_party/monocypher/monocypher-ed25519.c)
target_include_directories(monocypher PUBLIC third_party/monocypher)

# Ableton Link: vendored here, fetched by the source distribution
# (docs/dev/build.md). Only gui/LinkSync.cpp may include Link.hpp.
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
# SYSTEM: its payload keys are multi-character constants.
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
