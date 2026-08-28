# Helpers for building organism packs. See packs/sample/README.md.
#
# The tag mirrors PackLoader::platformTag(). On Apple it must come from
# CMAKE_OSX_ARCHITECTURES, not CMAKE_SYSTEM_PROCESSOR, which reports the HOST:
# cross-building for Intel would stamp "darwin-arm64" on an x86_64 pack and the
# app would boot fine and find no packs at all.
set(HUM_TARGET_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
  list(LENGTH CMAKE_OSX_ARCHITECTURES HUM_N_ARCH)
  if(HUM_N_ARCH GREATER 1)
    # One fat binary, two runtime tags, so it would have to be published under
    # both. Not wired up - fail loudly rather than mislay the packs.
    message(FATAL_ERROR
      "CMAKE_OSX_ARCHITECTURES=\"${CMAKE_OSX_ARCHITECTURES}\": universal builds are "
      "not supported yet, because a pack's binary lives under bin/<platform-tag>/ "
      "and PackLoader resolves that tag per slice at runtime. Build one arch at a "
      "time instead - e.g. `make build ARCH=x86_64`.")
  endif()
  set(HUM_TARGET_PROCESSOR "${CMAKE_OSX_ARCHITECTURES}")
endif()
string(TOLOWER "${CMAKE_SYSTEM_NAME}-${HUM_TARGET_PROCESSOR}" HUM_PLATFORM_TAG)

# Both arches explicitly: the linker ad-hoc signs arm64 by itself and leaves
# x86_64 unsigned, which is how the Intel pack.dylib once shipped unsigned.
function(hum_sign_pack_binary TARGET)
  if(NOT APPLE)
    return()
  endif()
  set(ID "${HUM_CODESIGN_IDENTITY}")
  if(NOT ID)
    set(ID "-")
  endif()
  add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND codesign --force --timestamp=none --sign "${ID}" "$<TARGET_FILE:${TARGET}>"
    COMMENT "Signing $<TARGET_FILE_NAME:${TARGET}> (identity: ${ID})"
    VERBATIM)
endfunction()

function(hum_add_pack_dyn PACK_ID)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/pack.json" HUM_PACK_JSON)
  configure_file("${HUM_SDK_DIR}/cmake/PackManifestJson.h.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/generated/PackManifestJson.h" @ONLY)

  add_library(hum_pack_${PACK_ID}_dyn SHARED "${CMAKE_CURRENT_SOURCE_DIR}/entry.cpp")
  target_include_directories(hum_pack_${PACK_ID}_dyn PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated")
  target_link_libraries(hum_pack_${PACK_ID}_dyn PRIVATE hum_pack_${PACK_ID})
  # RUNTIME too - a .dll is one on Windows. Not ARCHIVE: that is the import
  # library. The empty $<0:> stops a multi-config generator appending /$<CONFIG>
  # below the platform-tag dir PackLoader reads.
  set(HUM_PACK_DYN_OUT
      "${CMAKE_BINARY_DIR}/packs_dyn/${PACK_ID}/bin/${HUM_PLATFORM_TAG}$<0:>")
  set_target_properties(hum_pack_${PACK_ID}_dyn PROPERTIES
    OUTPUT_NAME "pack"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${HUM_PACK_DYN_OUT}"
    RUNTIME_OUTPUT_DIRECTORY "${HUM_PACK_DYN_OUT}")
  hum_sign_pack_binary(hum_pack_${PACK_ID}_dyn)
endfunction()

# Sets HUM_PYTHON for the .humpacks and for the hand-model rebuild in
# engine/CMakeLists.txt: one place knows that a bare "python3" on Windows is a
# WindowsApps Store alias that exits 9009. Cached, or every caller re-warns.
function(hum_find_python)
  if(NOT HUM_PYTHON)
    set(_hum_py "")
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_EXECUTABLE AND NOT Python3_EXECUTABLE MATCHES "[Ww]indows[Aa]pps")
      set(_hum_py "${Python3_EXECUTABLE}")
    endif()
    if(NOT _hum_py AND WIN32)
      find_program(HUM_PY_LAUNCHER py)
      if(HUM_PY_LAUNCHER)
        set(_hum_py "${HUM_PY_LAUNCHER}")
      endif()
    endif()
    if(_hum_py)
      set(HUM_PYTHON "${_hum_py}" CACHE INTERNAL "interpreter for .humpacks and the hand model")
    else()
      # Never cached, so installing Python later is enough on its own.
      get_property(_hum_py_warned GLOBAL PROPERTY HUM_PYTHON_WARNED)
      if(NOT _hum_py_warned)
        message(WARNING
          "no Python interpreter found - the .humpack bundles (and hum_tests, "
          "which loads them) cannot be built. Install Python 3 and re-configure; "
          "on Windows use python.org or `winget install Python.Python.3.12`, NOT "
          "the Microsoft Store alias on PATH.")
        set_property(GLOBAL PROPERTY HUM_PYTHON_WARNED ON)
      endif()
      set(HUM_PYTHON python3 PARENT_SCOPE)   # trust PATH where find_package is blind
    endif()
  endif()
endfunction()

function(hum_pack_bundle PACK_ID)
  if(NOT TARGET humpacks)
    add_custom_target(humpacks COMMENT "Building .humpack bundles")
  endif()
  hum_find_python()
  set(OUT "${CMAKE_BINARY_DIR}/humpacks/${PACK_ID}.humpack")
  # DEPENDS on the manifests and blueprints, not just the binary: without it,
  # editing one rebuilt nothing and the .humpack kept shipping the old layout.
  file(GLOB_RECURSE PACK_ASSETS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/organisms/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/help/*")
  add_custom_command(OUTPUT "${OUT}"
    COMMAND "${HUM_PYTHON}"
      "${HUM_SDK_DIR}/../tools/make_humpack.py"
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${CMAKE_BINARY_DIR}/packs_dyn/${PACK_ID}"
      "${OUT}"
    DEPENDS hum_pack_${PACK_ID}_dyn
      "${CMAKE_CURRENT_SOURCE_DIR}/pack.json"
      ${PACK_ASSETS}
    COMMENT "Packaging ${PACK_ID}.humpack")
  add_custom_target(humpack_${PACK_ID} DEPENDS "${OUT}")
  add_dependencies(humpacks humpack_${PACK_ID})
endfunction()
