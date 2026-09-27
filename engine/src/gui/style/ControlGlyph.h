// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/ControlMode.h"
#include "gui/style/Colours.h"

namespace hum {

inline void paintButtonGlyph(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c) {
    const auto box = r.withSizeKeepingCentre(r.getHeight() * 0.8f, r.getHeight() * 0.8f);
    g.setColour(c);
    g.drawRoundedRectangle(box, box.getHeight() * 0.2f, 1.3f);
    g.fillRoundedRectangle(box.reduced(box.getHeight() * 0.28f), box.getHeight() * 0.1f);
}

inline void paintFaderGlyph(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c) {
    const float h = r.getHeight() * 0.9f;
    const auto slot = r.withSizeKeepingCentre(1.6f, h);
    g.setColour(c.withAlpha(alpha::mid));
    g.fillRoundedRectangle(slot, 0.8f);
    g.setColour(c);
    const auto cap = r.withSizeKeepingCentre(h * 0.55f, h * 0.22f).translated(0.0f, h * 0.12f);
    g.fillRoundedRectangle(cap, 1.2f);
}

inline void paintEncoderGlyph(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c) {
    const float d = r.getHeight() * 0.72f;
    const auto knob = r.withSizeKeepingCentre(d, d);
    g.setColour(c);
    g.drawEllipse(knob, 1.3f);
    const auto centre = knob.getCentre();
    g.drawLine(centre.x, centre.y, centre.x, knob.getY() + 1.5f, 1.3f);
    juce::Path ring;
    const float rr = d * 0.5f + 2.2f;
    ring.addCentredArc(centre.x, centre.y, rr, rr, 0.0f, 0.5f, 2.6f, true);
    g.strokePath(ring, juce::PathStrokeType(1.0f));
    juce::Path head;
    const float ax = centre.x + rr * std::sin(2.6f);
    const float ay = centre.y - rr * std::cos(2.6f);
    head.addTriangle(ax - 2.2f, ay - 1.2f, ax + 1.6f, ay - 2.0f, ax + 0.2f, ay + 1.8f);
    g.fillPath(head);
}

inline void paintControlGlyph(juce::Graphics& g, ControlType type, juce::Rectangle<float> r,
                              juce::Colour c) {
    switch (type) {
        case ControlType::Button:  paintButtonGlyph(g, r, c); return;
        case ControlType::Fader:   paintFaderGlyph(g, r, c); return;
        case ControlType::Encoder: paintEncoderGlyph(g, r, c); return;
    }
}

}
