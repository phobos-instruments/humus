// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/net/ControlShape.h"
#include "gui/properties/ControlModeText.h"
#include "gui/properties/ModeChips.h"
#include "gui/style/Colours.h"
#include "gui/style/ControlGlyph.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class SourceTile : public juce::Component, public juce::SettableTooltipClient {
public:
    std::function<void()> onSelect;

    void setContent(const juce::String& name, const ControlShape& shape, std::vector<int> held,
                    bool learning) {
        name_ = name;
        shape_ = shape;
        held_ = std::move(held);
        learning_ = learning;
        repaint();
    }
    void setSelected(bool on) { if (on != selected_) { selected_ = on; repaint(); } }
    bool selected() const { return selected_; }
    void setActive(bool on) { if (on != active_) { active_ = on; repaint(); } }
    juce::String nameForTest() const { return name_; }
    juce::String modeForTest() const { return learning_ ? juce::String() : behaviourText(shape_); }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        if (selected_) paintChosen(g, r);
        else if (isMouseOver()) {
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(r, 4.0f);
        }
        r = r.reduced(10.0f, 0.0f);
        const auto dot = r.removeFromLeft(8.0f).withSizeKeepingCentre(6.0f, 6.0f);
        g.setColour(active_ ? Palette::accent : Palette::border);
        g.fillEllipse(dot);
        r.removeFromLeft(6.0f);
        const auto ink = selected_ ? Palette::accent : Palette::text;
        if (!learning_) {
            paintControlGlyph(g, shape_.type, r.removeFromLeft(18.0f).reduced(0.0f, 7.0f), ink);
            r.removeFromLeft(8.0f);
        }
        const auto mode = learning_ ? juce::String()
                          : shape_.isEncoder() ? modeText(shape_.encoder) : behaviourText(shape_);
        g.setFont(juce::FontOptions(11.5f));
        const float modeW = mode.isEmpty() ? 0.0f : juce::jmin(90.0f, 8.0f + (float) mode.length() * 6.5f);
        g.setColour(Palette::textDim);
        g.drawText(mode, r.removeFromRight(modeW), juce::Justification::centredRight);
        if (!held_.empty()) r = paintHeld(g, r);
        g.setColour(learning_ ? Palette::accent : ink);
        g.setFont(juce::FontOptions(12.5f));
        g.drawFittedText(name_, r.toNearestInt(), juce::Justification::centredLeft, 1);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu() && onSelect) onSelect();
    }
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }

private:
    juce::Rectangle<float> paintHeld(juce::Graphics& g, juce::Rectangle<float> r) const {
        g.setFont(juce::FontOptions(10.5f));
        for (int h : held_) {
            const auto text = tr("control-mode.hold-key", "hold") + " " + heldSourceText(h);
            const float w = 12.0f + (float) text.length() * 5.8f;
            auto pill = r.removeFromLeft(w).withSizeKeepingCentre(w, 18.0f);
            const float dashes[] = {3.0f, 2.0f};
            g.setColour(Palette::textDim);
            juce::Path outline;
            outline.addRoundedRectangle(pill, 9.0f);
            juce::PathStrokeType(1.0f).createDashedStroke(outline, outline, dashes, 2);
            g.fillPath(outline);
            g.setColour(Palette::text);
            g.drawText(text, pill, juce::Justification::centred);
            r.removeFromLeft(6.0f);
        }
        return r;
    }

    juce::String name_;
    ControlShape shape_;
    std::vector<int> held_;
    bool learning_ = false, selected_ = false, active_ = false;
};

}
