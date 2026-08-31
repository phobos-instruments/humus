# Curl-less Linux POSTs framed one CRLF early, so every body arrived two
# bytes short (docs/dev/build.md, "JUCE, and the three patches"). Idempotent;
#   cmake -DJUCE_SOURCE_DIR=engine/build/_deps/juce-src -P <this file>

if(NOT DEFINED JUCE_SOURCE_DIR)
  message(FATAL_ERROR "pass -DJUCE_SOURCE_DIR=<juce checkout>")
endif()

set(_f "${JUCE_SOURCE_DIR}/modules/juce_core/native/juce_Network_linux.cpp")
if(NOT EXISTS "${_f}")
  message(FATAL_ERROR "not a JUCE tree: ${_f} missing")
endif()

file(READ "${_f}" _src)

if(_src MATCHES "HUMUS-PATCH")
  message(STATUS "JUCE Linux POST header patch already applied")
  return()
endif()

set(_old "        if (userHeaders.isNotEmpty())
            header << \"\\r\\n\" << userHeaders;

        header << \"\\r\\n\\r\\n\"")
set(_new "        // HUMUS-PATCH: trimEnd, or the newline these headers already end
        // with plus the terminator below is one blank line too many and the
        // body starts two bytes into the CRLF. See
        // engine/cmake/PatchJuceNetworkLinux.cmake.
        if (userHeaders.isNotEmpty())
            header << \"\\r\\n\" << userHeaders.trimEnd();

        header << \"\\r\\n\\r\\n\"")

string(FIND "${_src}" "${_old}" _at)
if(_at EQUAL -1)
  message(FATAL_ERROR "createRequestHeader anchor not found - JUCE version changed, revisit this patch")
endif()
string(REPLACE "${_old}" "${_new}" _src "${_src}")

file(WRITE "${_f}" "${_src}")
message(STATUS "JUCE Linux POST header patch applied (no stray blank line before the body)")
