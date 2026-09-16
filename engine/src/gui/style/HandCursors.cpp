// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/style/HandCursors.h"

#if !JUCE_MAC

namespace hum {

namespace {

constexpr int kHandScale = 2;
constexpr int kHandSide = 16;
constexpr float kOutline = 1.15f;

juce::Path closedHandPath() {
    juce::Path fist;
    fist.addRoundedRectangle(3.0f, 6.8f, 10.0f, 7.4f, 2.6f);
    fist.addEllipse(4.0f, 4.9f, 3.2f, 3.2f);
    fist.addEllipse(6.6f, 4.4f, 3.2f, 3.2f);
    fist.addEllipse(9.2f, 4.9f, 3.2f, 3.2f);
    fist.addEllipse(1.7f, 7.9f, 3.6f, 3.6f);
    return fist;
}

juce::Image closedHandImage() {
    juce::Image out(juce::Image::ARGB, kHandSide * kHandScale, kHandSide * kHandScale, true);
    juce::Graphics g(out);
    g.addTransform(juce::AffineTransform::scale((float) kHandScale));
    const auto fist = closedHandPath();
    g.setColour(juce::Colours::black);
    g.strokePath(fist, juce::PathStrokeType(2.0f * kOutline, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
    g.setColour(juce::Colours::white);
    g.fillPath(fist);
    g.setColour(juce::Colours::black.withAlpha(alpha::mid));
    for (const float x : {6.4f, 9.0f}) g.drawLine(x, 5.6f, x, 7.4f, 0.7f);
    return out;
}

}

juce::MouseCursor grabbingHandCursor() {
    static const juce::MouseCursor cursor(juce::ScaledImage(closedHandImage(), kHandScale),
                                          {8, 7});
    return cursor;
}

}

#endif
