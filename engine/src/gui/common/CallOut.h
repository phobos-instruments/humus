// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

using ClassFilter = std::function<bool(const std::string& className)>;

struct PickerScope {
    juce::Component* parent = nullptr;
    ClassFilter keep;
};

inline void launchCallOut(std::unique_ptr<juce::Component> content, juce::Rectangle<int> screenAnchor,
                          juce::Component* parent) {
    const auto anchor = parent != nullptr ? parent->getLocalArea(nullptr, screenAnchor) : screenAnchor;
    juce::CallOutBox::launchAsynchronously(std::move(content), anchor, parent);
}

}
