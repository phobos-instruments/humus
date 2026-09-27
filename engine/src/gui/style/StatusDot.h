// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "hum/dsp/DspMath.h"

namespace hum {

struct StatusDot {
    juce::String label;
    juce::Colour tint{ink::state::recording};
    bool pulses = true;

    static constexpr int kDot = 8;
    static constexpr int kGap = 5;
    static constexpr int kHeight = 16;
    static constexpr float kPulseHz = 1.2f;
    static constexpr float kPulseFloor = 0.45f;

    static StatusDot rec() {
        return {tr("status-dot.rec", "REC"), ink::state::recording, true};
    }

    static StatusDot armed() {
        return {tr("status-dot.armed", "ARM"), ink::state::armed, false};
    }

    static StatusDot live() {
        return {tr("status-dot.live", "LIVE"), ink::state::ok, false};
    }

    static constexpr int kGlyph = 7;

    int width() const {
        return kDot + kGap * 3 + kGlyph * juce::jmax(3, label.length());
    }

    juce::Rectangle<int> boundsIn(juce::Rectangle<int> host, bool topRight = true) const {
        const auto r = host.reduced(6).removeFromTop(kHeight).withWidth(width());
        return topRight ? r.withRightX(host.getRight() - 6) : r;
    }

    void paint(juce::Graphics& g, juce::Rectangle<int> where) const {
        const float wash = pulses ? kPulseFloor
                                        + (1.0f - kPulseFloor)
                                              * (0.5f + 0.5f * std::sin(
                                                            (float) (juce::Time::getMillisecondCounter()
                                                                     * 0.001 * kPulseHz * kTwoPi)))
                                  : 1.0f;
        g.setColour(juce::Colours::black.withAlpha(alpha::dim));
        g.fillRoundedRectangle(where.toFloat(), 3.0f);
        auto lamp = where.reduced(kGap, (kHeight - kDot) / 2).removeFromLeft(kDot);
        g.setColour(tint.withMultipliedAlpha(wash));
        g.fillEllipse(lamp.toFloat());
        g.setColour(Palette::text.withAlpha(alpha::heavy));
        g.setFont(juce::FontOptions(10.5f));
        g.drawText(label, where.withTrimmedLeft(kDot + kGap * 2), juce::Justification::centredLeft,
                   false);
    }

    void paintIn(juce::Graphics& g, juce::Rectangle<int> host, bool topRight = true) const {
        paint(g, boundsIn(host, topRight));
    }
};

}
