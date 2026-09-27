// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/LookAndFeel.h"

namespace hum {

class NumberPrompt : public juce::Component {
public:
    static void show(juce::Rectangle<int> anchor, const juce::String& title,
                     const juce::String& allowed, int value, std::function<void(int)> onPick) {
        auto owned = std::make_unique<NumberPrompt>(title, allowed, value, std::move(onPick));
        auto* raw = owned.get();
        auto& box = juce::CallOutBox::launchAsynchronously(std::move(owned), anchor, nullptr);
        raw->box_ = &box;
    }

    NumberPrompt(const juce::String& title, const juce::String& allowed, int value,
                 std::function<void(int)> onPick)
        : onPick_(std::move(onPick)) {
        title_.setText(title, juce::dontSendNotification);
        title_.setColour(juce::Label::textColourId, Palette::textDim);
        addAndMakeVisible(title_);
        field_.setInputRestrictions(4, allowed);
        field_.setJustification(juce::Justification::centred);
        field_.setText(juce::String(value), juce::dontSendNotification);
        field_.onReturnKey = [this] { commit(); };
        addAndMakeVisible(field_);
        okBtn_.onClick = [this] { commit(); };
        addAndMakeVisible(okBtn_);
        setSize(236, 88);
    }

    void visibilityChanged() override {
        if (isShowing()) { field_.grabKeyboardFocus(); field_.selectAll(); }
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        title_.setBounds(r.removeFromTop(18));
        r.removeFromTop(4);
        field_.setBounds(r.removeFromTop(24).removeFromLeft(72));
        r.removeFromTop(8);
        okBtn_.setBounds(r.removeFromTop(24).removeFromRight(72));
    }

private:
    void commit() {
        const int v = field_.getText().getIntValue();
        auto cb = onPick_;
        if (box_ != nullptr) box_->dismiss();
        if (cb) cb(v);
    }

    juce::Label title_;
    juce::TextEditor field_;
    juce::TextButton okBtn_{"OK"};
    std::function<void(int)> onPick_;
    juce::CallOutBox* box_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberPrompt)
};

}
