# JUCE 8.0.4 opens every window on Direct2D, where Graphics::drawImage draws
# nothing at all - no image in the app is visible on Windows. Index 0 is the
# software renderer JUCE used before 8. Re-test on other hardware: if it is a
# driver bug rather than a JUCE one, a narrower fix may be right.
#
# Idempotent. Runs as FetchContent's PATCH_COMMAND, or by hand:
#   cmake -DJUCE_SOURCE_DIR=engine/build/_deps/juce-src -P engine/cmake/PatchJuceDirect2D.cmake

if(NOT DEFINED JUCE_SOURCE_DIR)
  message(FATAL_ERROR "pass -DJUCE_SOURCE_DIR=<juce checkout>")
endif()

set(_f "${JUCE_SOURCE_DIR}/modules/juce_gui_basics/native/juce_Windowing_windows.cpp")
if(NOT EXISTS "${_f}")
  message(FATAL_ERROR "not a JUCE tree: ${_f} missing")
endif()

file(READ "${_f}" _src)

if(_src MATCHES "HUMUS-PATCH")
  message(STATUS "JUCE Direct2D renderer patch already applied")
  return()
endif()

# Trailing argument = engine index into
# contextDescriptorList<GDIRenderContext, D2DRenderContext>.
set(_old "ComponentPeer* Component::createNewPeer (int styleFlags, void* parentHWND)
{
    return new HWNDComponentPeer { *this, styleFlags, (HWND) parentHWND, false, 1 };
}")
set(_new "ComponentPeer* Component::createNewPeer (int styleFlags, void* parentHWND)
{
    // HUMUS-PATCH: default 0 (software), not 1 (Direct2D) - drawImage draws
    // nothing under D2D in 8.0.4. HUMUS_RENDERER=d2d puts the stock choice
    // back for one run, so the two can be compared on one machine without a
    // rebuild. See engine/cmake/PatchJuceDirect2D.cmake.
    static const int humusEngine = []
    {
        // .trim() is load-bearing: `set VAR=d2d & app` keeps everything up
        // to the ampersand, so cmd hands us \"d2d \" and an untrimmed
        // compare silently falls back to software.
        const auto v = SystemStats::getEnvironmentVariable (\"HUMUS_RENDERER\", {}).trim();
        return (v.equalsIgnoreCase (\"d2d\") || v.equalsIgnoreCase (\"direct2d\")) ? 1 : 0;
    }();
    return new HWNDComponentPeer { *this, styleFlags, (HWND) parentHWND, false, humusEngine };
}")
string(FIND "${_src}" "${_old}" _at)
if(_at EQUAL -1)
  message(FATAL_ERROR "createNewPeer anchor not found - JUCE version changed, revisit this patch")
endif()
string(REPLACE "${_old}" "${_new}" _src "${_src}")

file(WRITE "${_f}" "${_src}")
message(STATUS "JUCE Direct2D renderer patch applied (windows use the software renderer)")
