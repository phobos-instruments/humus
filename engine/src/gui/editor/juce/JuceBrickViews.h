// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/juce/JuceBasicBrickViews.h"
#include "gui/host/EditorHost.h"

namespace hum {

class JuceBrickViews : public JuceBasicBrickViews {
public:
    JuceBrickViews(juce::Component& parent, EditorHost& host) : JuceBasicBrickViews(parent), host_(host) {}

    std::unique_ptr<RichView> rich(const LayoutSpec& spec, const LayoutSpec::Control& control,
                                   RichOwner& owner) override;

private:
    EditorHost& host_;
};

}
