// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/net/ControlShape.h"
#include "gui/properties/ControlModeText.h"
#include "gui/properties/ModeChips.h"
#include "gui/style/Colours.h"
#include "gui/style/ControlGlyph.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class ControlKindButton : public juce::TextButton {
public:
    void setShape(const ControlShape& s) {
        shape_ = s;
        setButtonText(behaviourText(s));
        setTooltip(modeText(s.type) + " - " + behaviourText(s) + "\n"
                   + tr("control-mode.kind-tooltip", "Click to change how the control behaves"));
        repaint();
    }
    const ControlShape& shape() const { return shape_; }
    void showAsLearning(const juce::String& text) {
        learning_ = true;
        setButtonText(text);
        setTooltip({});
        repaint();
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        const bool on = getToggleState();
        getLookAndFeel().drawButtonBackground(
            g, *this, findColour(on ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId),
            over, down);
        if (on) paintChosen(g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced(6.0f, 4.0f);
        const auto ink = on ? Palette::accent : findColour(juce::TextButton::textColourOffId);
        if (!learning_) {
            paintControlGlyph(g, shape_.type, r.removeFromLeft(r.getHeight()), ink);
            r.removeFromLeft(5.0f);
        }
        g.setColour(ink);
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(getButtonText(), r.toNearestInt(), juce::Justification::centredLeft, 1);
    }

private:
    ControlShape shape_;
    bool learning_ = false;
};

class HeldChips : public juce::Component, public juce::SettableTooltipClient {
public:
    void setHeld(std::vector<int> held) {
        held_ = std::move(held);
        juce::StringArray names;
        for (int h : held_) names.add(heldSourceText(h));
        setTooltip(tr("control-mode.held-tooltip", "Works only while this is held:") + " "
                   + names.joinIntoString(" + "));
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(0.0f, 3.0f);
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(Palette::textDim);
        const auto lead = tr("control-mode.while", "while");
        g.drawText(lead, r.removeFromLeft(34.0f), juce::Justification::centredLeft, false);
        for (int h : held_) {
            const auto text = heldSourceText(h);
            const float w = juce::jmin(r.getWidth(), 10.0f + (float) text.length() * 6.5f);
            if (w < 14.0f) break;
            auto pill = r.removeFromLeft(w);
            g.setColour(Palette::accent.withAlpha(alpha::veil));
            g.fillRoundedRectangle(pill, pill.getHeight() * 0.5f);
            g.setColour(Palette::accent);
            g.drawRoundedRectangle(pill.reduced(0.5f), pill.getHeight() * 0.5f, 1.0f);
            g.setColour(Palette::text);
            g.drawFittedText(text, pill.toNearestInt(), juce::Justification::centred, 1);
            r.removeFromLeft(3.0f);
        }
        g.setColour(Palette::textDim);
        g.drawText("+", r.removeFromLeft(10.0f), juce::Justification::centred, false);
    }

private:
    std::vector<int> held_;
};

}
