#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

#if JUCE_MAC
bool dragFilesWithImage(const juce::StringArray& files, juce::Component* source,
                        const juce::Image& chip, std::function<void()> onDone);
#else
inline bool dragFilesWithImage(const juce::StringArray&, juce::Component*,
                               const juce::Image&, std::function<void()>) {
    return false;
}
#endif

}
