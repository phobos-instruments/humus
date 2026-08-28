#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

#if JUCE_MAC
juce::MouseCursor grabbingHandCursor();
#else
inline juce::MouseCursor grabbingHandCursor() {
    return juce::MouseCursor::DraggingHandCursor;
}
#endif

}
