// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class ViewSwitch : public juce::Component, public juce::SettableTooltipClient {
public:
    explicit ViewSwitch(bool knobsUi = true) : knobsUi_(knobsUi) {
        if (knobsUi_) setTooltip(tr("switch.view-generic-knobs-the-plugin", "View: generic knobs / the plugin's own UI"));
    }

    std::function<void(bool second)> onChange;
    void set(bool second) { second_ = second; repaint(); }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(Palette::background.darker(0.2f));
        g.fillRoundedRectangle(r, 4.0f);
        auto left = r;
        drawSeg(g, left.removeFromLeft(r.getWidth() * 0.5f), !second_, true);
        drawSeg(g, left, second_, false);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        const bool second = e.x >= getWidth() / 2;
        if (second == second_) return;
        set(second);
        if (onChange) onChange(second_);
    }

private:
    void drawSeg(juce::Graphics& g, juce::Rectangle<float> r, bool active, bool first) {
        if (active) {
            g.setColour(Palette::accent.withAlpha(alpha::scrim));
            g.fillRoundedRectangle(r.reduced(1.5f), 3.0f);
        }
        const auto ink = active ? Palette::accent : Palette::textDim;
        drawIconGlyph(g, knobsUi_ ? (first ? IconGlyph::KnobView : IconGlyph::PluginView)
                                  : (first ? IconGlyph::RackView : IconGlyph::FreeView),
                      r.withSizeKeepingCentre(11.0f, 11.0f), ink, true);
    }

    bool knobsUi_ = true;
    bool second_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewSwitch)
};

class FoldButton : public juce::Button {
public:
    FoldButton() : juce::Button({}) { setCollapsed(false); }

    void setCollapsed(bool c) {
        collapsed_ = c;
        setTooltip(c ? tr("switch.expand-to-the-full-editor", "Expand to the full editor")
                     : tr("switch.collapse-to-a-compact-device", "Collapse to a compact device strip"));
        repaint();
    }

    void paintButton(juce::Graphics& g, bool over, bool) override {
        const auto c = getLocalBounds().toFloat().getCentre();
        g.setColour(over && isEnabled() ? Palette::accent : Palette::textDim);
        juce::Path p;
        if (collapsed_) {
            p.startNewSubPath(c.x - 1.5f, c.y - 4.0f);
            p.lineTo(c.x + 3.0f, c.y);
            p.lineTo(c.x - 1.5f, c.y + 4.0f);
        } else {
            p.startNewSubPath(c.x - 4.0f, c.y - 1.5f);
            p.lineTo(c.x, c.y + 3.0f);
            p.lineTo(c.x + 4.0f, c.y - 1.5f);
        }
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    }

private:
    bool collapsed_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FoldButton)
};

class FloatButton : public juce::Button {
public:
    FloatButton() : juce::Button({}) {
        setTooltip(tr("switch.float-open-the-plugin-s", "Float: open the plugin's UI in its own window"));
    }

    void paintButton(juce::Graphics& g, bool over, bool) override {
        auto r = getLocalBounds().toFloat().reduced(5.5f);
        g.setColour(over && isEnabled() ? Palette::accent : Palette::text);
        g.drawRect(r.getX(), r.getCentreY(), r.getWidth() * 0.6f, r.getHeight() * 0.5f, 1.2f);
        g.drawLine(r.getCentreX(), r.getCentreY(), r.getRight(), r.getY(), 1.5f);
        g.drawLine(r.getRight() - r.getWidth() * 0.4f, r.getY(), r.getRight(), r.getY(), 1.5f);
        g.drawLine(r.getRight(), r.getY(), r.getRight(), r.getY() + r.getHeight() * 0.4f, 1.5f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatButton)
};

}
