// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "hum/StepChance.h"

namespace hum::stepgrid {

inline constexpr int kChanceKeyW = 22;
inline constexpr int kStepsView = 0;
inline constexpr int kChanceView = 1;

inline int viewOf(double value) { return (int) std::lround(value); }

inline juce::String percentText(int chance) { return juce::String(chance) + "%"; }

inline juce::String chanceKeyTip() {
    return tr("step-grid.chance-key", "Step chance: click, then drag a step up or down to set how often it plays");
}

inline void paintChanceKey(juce::Graphics& g, juce::Rectangle<float> area, bool on, juce::Colour fam) {
    g.setColour(on ? fam : Palette::panelLight);
    g.fillRoundedRectangle(area, 3.0f);
    g.setColour(on ? fam.brighter(0.4f) : Palette::border);
    g.drawRoundedRectangle(area.reduced(0.5f), 3.0f, 1.0f);
    g.setColour(on ? Palette::background : Palette::textDim);
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.drawText("%", area, juce::Justification::centred, false);
}

inline void paintChanceFade(juce::Graphics& g, juce::Rectangle<float> area, int chance, juce::Colour ground) {
    if (chance >= kFullChance) return;
    g.setColour(ground.withAlpha(alpha::strong * (1.0f - (float) chance / (float) kFullChance)));
    g.fillRoundedRectangle(area, 2.5f);
}

inline void paintReadout(juce::Graphics& g, const juce::String& text, float cx, float anchorY, float width,
                         juce::Colour edge) {
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    const float w = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), text) + 12.0f;
    const float h = 18.0f;
    const float x = std::clamp(cx - w * 0.5f, 2.0f, std::max(2.0f, width - w - 2.0f));
    float y = anchorY - h - 8.0f;
    if (y < 2.0f) y = anchorY + 8.0f;
    const juce::Rectangle<float> pill(x, y, w, h);
    g.setColour(Palette::background.withAlpha(alpha::nearOpaque));
    g.fillRoundedRectangle(pill, 4.0f);
    g.setColour(edge.brighter(0.4f));
    g.drawRoundedRectangle(pill, 4.0f, 1.0f);
    g.setColour(Palette::text);
    g.drawText(text, pill, juce::Justification::centred);
}

inline void paintChanceBar(juce::Graphics& g, juce::Rectangle<float> area, int chance, juce::Colour fam) {
    g.setColour(Palette::background.withAlpha(alpha::mid));
    g.fillRoundedRectangle(area, 2.5f);
    const float h = area.getHeight() * (float) chance / (float) kFullChance;
    g.setColour(fam.withAlpha(alpha::strong));
    g.fillRoundedRectangle(area.withTop(area.getBottom() - h), 2.5f);
}

inline void paintChanceTag(juce::Graphics& g, juce::Rectangle<float> area, int chance, juce::Colour ink) {
    if (chance >= kFullChance) return;
    g.setFont(juce::FontOptions(8.5f).withStyle("Bold"));
    g.setColour(ink);
    g.drawText(juce::String(chance), area, juce::Justification::centredTop, false);
}

}
