// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/Contrast.h"
#include "gui/style/LookAndFeel.h"

namespace hum::drop {

inline constexpr float kBadgeSize = 18.0f;
inline constexpr float kBadgeInset = 4.0f;
inline constexpr float kWideBadgeSize = 32.0f;

inline juce::Rectangle<float> badgeArea(juce::Rectangle<float> target) {
    const float size = juce::jmin(kBadgeSize, target.getHeight() - 2.0f * kBadgeInset);
    return {target.getRight() - kBadgeInset - size, target.getY() + kBadgeInset, size, size};
}

inline void paintBadgeAt(juce::Graphics& g, juce::Rectangle<float> badge) {
    if (badge.getWidth() <= 4.0f) return;
    g.setColour(Palette::background.withAlpha(alpha::mid));
    g.fillEllipse(badge.expanded(1.5f));
    g.setColour(Palette::accent);
    g.fillEllipse(badge);
    const auto c = badge.getCentre();
    const float arm = badge.getWidth() * 0.27f;
    const float thick = juce::jmax(1.5f, badge.getWidth() * 0.13f);
    g.setColour(contrast::readable(Palette::accent, Palette::background, juce::Colours::white));
    g.drawLine(c.x - arm, c.y, c.x + arm, c.y, thick);
    g.drawLine(c.x, c.y - arm, c.x, c.y + arm, thick);
}

inline void paintBadge(juce::Graphics& g, juce::Rectangle<float> target) { paintBadgeAt(g, badgeArea(target)); }

inline void paintCornerBadge(juce::Graphics& g, juce::Rectangle<float> target) {
    paintBadgeAt(g, juce::Rectangle<float>(kBadgeSize, kBadgeSize).withCentre(target.getTopRight()));
}

inline juce::Rectangle<float> wideBadgeArea(juce::Rectangle<float> target) {
    return juce::Rectangle<float>(kWideBadgeSize, kWideBadgeSize).withCentre(target.getCentre());
}

inline void paintOutline(juce::Graphics& g, juce::Rectangle<float> target, float corner) {
    g.setColour(Palette::accent);
    g.drawRoundedRectangle(target.reduced(1.0f), corner, 2.0f);
}

inline void paintTarget(juce::Graphics& g, juce::Rectangle<float> target, float corner = 4.0f) {
    paintOutline(g, target, corner);
    paintBadge(g, target);
}

inline void paintWideTarget(juce::Graphics& g, juce::Rectangle<float> target, float corner = 4.0f) {
    paintOutline(g, target, corner);
    paintBadgeAt(g, wideBadgeArea(target));
}

}
