# JUCE 8.0.4 writes one blank line too many between the headers and the body of
# every POST it sends on Linux without libcurl, and we build with
# JUCE_USE_CURL=0. URL::createHeadersAndPostData always ends the header block
# with "Content-length: N\r\n", then createRequestHeader appends "\r\n\r\n" on
# top of that trailing newline. The server reads the extra CRLF as the end of
# the headers, so the body arrives as "\r\n" + JSON while Content-Length still
# counts only the JSON - two bytes short. Every hub POST (checkin, license
# activation, telemetry) came back 422 with an unterminated string.
#
# Idempotent. Runs as FetchContent's PATCH_COMMAND, or by hand:
#   cmake -DJUCE_SOURCE_DIR=engine/build/_deps/juce-src -P engine/cmake/PatchJuceNetworkLinux.cmake

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
