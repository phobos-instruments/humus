// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/graph/CordTrace.h"
#include "core/packs/Categories.h"
#include "core/params/ParamUnit.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/tracks/TracksLayout.h"

namespace hum::timelinechrome {

inline float textWidth(const juce::Font& f, const juce::String& text) {
    return juce::GlyphArrangement::getStringWidth(f, text);
}

inline constexpr float kChipFont = 11.0f;

inline constexpr float kCrumbFont = 11.5f;

inline void paintSnapChip(juce::Graphics& g, juce::Rectangle<int> r,
                          double gridBeats, bool chosen, const char* labelOverride = nullptr) {
    if (r.getWidth() < 24 || r.getHeight() < 8) return;
    const auto label = labelOverride ? std::string(labelOverride) : trackslayout::gridLabel(gridBeats);
    g.setColour(Palette::panelLight);
    g.fillRoundedRectangle(r.toFloat(), 2.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 2.0f, 1.0f);
    g.setColour(chosen ? Palette::accent : Palette::textDim);
    g.setFont(juce::FontOptions(kChipFont));
    g.drawText(label, r, juce::Justification::centred);
    if (chosen) g.fillEllipse((float) r.getRight() - 5.0f, (float) r.getY() + 2.0f, 3.0f, 3.0f);
}

inline juce::Colour laneAccent(const PatchDocumentModel& m, const std::string& node) {
    const auto dest = cords::destinationOf(m, node);
    const auto* dcm = dest.empty() ? nullptr : m.byName(dest);
    if (dcm == nullptr) return Palette::accent;
    return Palette::familyAccent(familyOf(dcm->displayClass));
}

inline void paintDestChip(juce::Graphics& g, juce::Rectangle<int> r,
                          const PatchDocumentModel& m, const std::string& node) {
    const auto dest = cords::destinationOf(m, node);
    if (dest.empty() || r.getWidth() < 26) return;
    const auto* dcm = m.byName(dest);
    const auto label = juce::String::fromUTF8("\xe2\x86\x92 ")
                       + juce::String(dcm != nullptr && !pseudoOwnerLabel(dcm->displayClass).empty()
                                          ? pseudoOwnerLabel(dcm->displayClass)
                                          : dest);
    g.setFont(juce::FontOptions(9.5f));
    const int wanted = (int) textWidth(g.getCurrentFont(), label) + 8;
    r = r.withWidth(juce::jmin(r.getWidth(), juce::jmax(26, wanted)));
    g.setColour(Palette::border);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 2.0f, 1.0f);
    g.setColour(Palette::textDim);
    g.drawText(label, r.reduced(3, 0), juce::Justification::centredLeft, true);
}

inline juce::String midiDestLabel(const PatchDocumentModel& m, const std::string& node) {
    std::string dest;
    for (const auto& c : m.midiConnections)
        if (c.src == node && c.srcOutlet == 0) { dest = c.dst; break; }
    return juce::String::fromUTF8("\xe2\x86\x92 ")
           + (dest.empty() ? juce::String("(nothing)") : juce::String(dest))
           + juce::String::fromUTF8("  \xe2\x96\xbe");
}

inline juce::Rectangle<int> midiDestChipRect(juce::Rectangle<int> r,
                                             const PatchDocumentModel& m,
                                             const std::string& node) {
    const juce::Font f{juce::FontOptions(12.0f)};
    const int wanted = (int) textWidth(f, midiDestLabel(m, node)) + 8;
    return r.withWidth(juce::jmin(r.getWidth(), juce::jmax(26, wanted)));
}

inline void paintMidiDestChip(juce::Graphics& g, juce::Rectangle<int> r,
                              const PatchDocumentModel& m, const std::string& node) {
    if (r.getWidth() < 26) return;
    std::string dest;
    for (const auto& c : m.midiConnections)
        if (c.src == node && c.srcOutlet == 0) { dest = c.dst; break; }
    const auto label = midiDestLabel(m, node);
    g.setFont(juce::FontOptions(12.0f));
    r = midiDestChipRect(r, m, node);
    g.setColour(Palette::panelLight);
    g.fillRoundedRectangle(r.toFloat(), 2.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 2.0f, 1.0f);
    g.setColour(dest.empty() ? Palette::textDim : Palette::accent);
    g.drawText(label, r.reduced(3, 0), juce::Justification::centredLeft, true);
}

inline void paintHeldBadge(juce::Graphics& g, juce::Rectangle<int> r, bool on = true) {
    if (r.getWidth() < 9 || r.getHeight() < 9) return;
    const auto amber = Palette::warnAmber();
    g.setColour(on ? amber : Palette::panelLight);
    g.fillRect(r);
    g.setColour(on ? amber.darker(0.9f) : Palette::textDim);
    g.setFont(juce::FontOptions(9.5f));
    g.drawText("H", r, juce::Justification::centred);
}

inline juce::String autoPointLabel(const std::string& kind, double value, double valueMax,
                                   Unit unit, double lo, double hi, const juce::String& position) {
    if (kind == "trigger") return position;
    const juce::String suffix = unitSuffix(unit);
    auto one = [&](double v) {
        juce::String s = unitPlain(unit, v, lo, hi);
        if (suffix.isNotEmpty()) s << " " << suffix;
        return s;
    };
    juce::String text = kind == "range" ? one(value) + " .. " + one(valueMax) : one(value);
    if (position.isNotEmpty()) text << "   " << position;
    return text;
}

inline void paintPointReadout(juce::Graphics& g, juce::Rectangle<int> lane, float px, float py,
                              const juce::String& text) {
    if (text.isEmpty() || lane.getWidth() < 40) return;
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.5f,
                                juce::Font::plain));
    const int w = (int) std::ceil(textWidth(g.getCurrentFont(), text)) + 10;
    const int h = 15;
    int x = (int) px + 8;
    if (x + w > lane.getRight()) x = (int) px - 8 - w;
    x = std::max(lane.getX(), x);
    int y = (int) py - h - 6;
    if (y < lane.getY()) y = std::min(lane.getBottom() - h, (int) py + 6);
    const juce::Rectangle<int> r(x, y, w, h);
    g.setColour(Palette::background.withAlpha(alpha::nearOpaque));
    g.fillRoundedRectangle(r.toFloat(), 3.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 3.0f, 1.0f);
    g.setColour(Palette::text);
    g.drawText(text, r, juce::Justification::centred, false);
}

}
