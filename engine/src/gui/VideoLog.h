#pragma once
#include <cstdlib>
#include <iostream>

#include <juce_core/juce_core.h>

namespace hum {

inline bool videoLogOn() {
    static const bool on = std::getenv("HUMUS_VIDEO_DEBUG") != nullptr;
    return on;
}

inline void videoLog(const juce::String& line) {
    if (videoLogOn()) std::cout << "[video] " << line << std::endl;
}

}
