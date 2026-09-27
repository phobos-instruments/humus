// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/pianoroll/PianoRollEditor.h"

#include <set>
#include <algorithm>
#include <cmath>

#include "gui/pianoroll/NoteColourMenu.h"
#include "gui/pianoroll/NoteEdit.h"

#include "gui/style/LookAndFeel.h"

#include "hum/dsp/DspMath.h"
#include "core/midi/MidiFormat.h"

namespace hum {

void PianoRollEditor::paint(juce::Graphics& g) {
    g.fillAll(Palette::background);
    paintGrid(g);
    paintNotes(g);
    paintCutGuide(g);
    paintVelocity(g);
    paintRuler(g);
    paintKeys(g);

    if (const auto head = playhead(); head.playing && head.tick >= 0.0) {
        const float x = geometry().tickToX(head.tick);
        g.setColour(Palette::accent.withAlpha(head.preview ? alpha::muted : alpha::heavy));
        g.drawLine(x, (float) gridTop(), x, (float) gridBottom(), head.preview ? 1.0f : 1.5f);
    }

    g.setColour(Palette::border);
    g.drawHorizontalLine(kToolbarH - 1, 0.0f, (float) getWidth());
}

void PianoRollEditor::paintKeys(juce::Graphics& g) {
    const int bottom = gridBottom();
    std::set<int> lit;
    if (keyNote_ >= 0) lit.insert(keyNote_);
    if (const double tick = playheadClipTick(); tick >= 0.0)
        for (const auto& e : model_.notes())
            if (tick >= e.tick && tick < e.tick + std::max(1, e.lengthTicks))
                lit.insert(e.pitch);
    g.setColour(Palette::panel);
    g.fillRect(0, gridTop(), kKeyW, bottom - gridTop());
    g.saveState();
    g.reduceClipRegion(0, gridTop(), kKeyW, bottom - gridTop());
    for (int y = gridTop(), pitch = model_.topPitch; y < bottom && pitch >= 0; y += kRowH, --pitch) {
        if (pitch > kMidiMax) continue;
        noteedit::paintPianoKey(g, {0.0f, (float) y, (float) (kKeyW - 1), (float) kRowH},
                                pitch, lit.count(pitch) != 0, Palette::accent);
        if (((pitch % 12) + 12) % 12 == 0) {
            g.setColour(juce::Colour(noteedit::kEbony));
            g.setFont(juce::Font(juce::FontOptions(9.0f)));
            g.drawText("C" + juce::String(pitch / 12 - 1), 2, y - 1, kKeyW - 5, kRowH + 2,
                       juce::Justification::centredRight, false);
        }
    }
    g.restoreState();
    g.setColour(Palette::border);
    g.drawVerticalLine(kKeyW - 1, (float) gridTop(), (float) bottom);
}

void PianoRollEditor::paintRuler(juce::Graphics& g) {
    const auto geo = geometry();
    const int dur = durationTicks();
    g.setColour(Palette::panel);
    g.fillRect(kKeyW, kToolbarH, getWidth() - kKeyW, kRulerH);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    const int barTicks = geo.barTicks;
    for (int t = 0, bar = 1; t < dur; t += barTicks, ++bar) {
        const float x = geo.tickToX(t);
        g.setColour(Palette::border);
        g.drawVerticalLine((int) x, (float) kToolbarH, (float) (kToolbarH + kRulerH));
        g.setColour(Palette::textDim);
        g.drawText(juce::String(bar), (int) x + 3, kToolbarH + 2, 30, kRulerH - 4,
                   juce::Justification::centredLeft, false);
    }
}

void PianoRollEditor::paintGrid(juce::Graphics& g) {
    const auto geo = geometry();
    const int dur = durationTicks();
    const int bottom = gridBottom();

    for (int y = gridTop(), pitch = model_.topPitch; y < bottom && pitch >= 0; y += kRowH, --pitch) {
        if (pitch > kMidiMax) continue;
        g.setColour(isBlackKey(pitch) ? Palette::background.brighter(0.03f)
                                      : Palette::background.brighter(0.07f));
        g.fillRect(kKeyW, y, getWidth() - kKeyW, kRowH);
        g.setColour(Palette::border.withAlpha(noteedit::laneRuleAlpha(pitch)));
        g.drawHorizontalLine(y + kRowH - 1, (float) kKeyW, (float) getWidth());
    }

    const int snap = snapTicks();
    for (int t = 0; t <= dur; t += snap) {
        const float x = geo.tickToX(t);
        const bool bar = t % (4 * Pattern::kTicksPerBeat) == 0;
        const bool beat = t % Pattern::kTicksPerBeat == 0;
        g.setColour(bar ? Palette::border : beat ? Palette::border.withAlpha(alpha::mid)
                                                 : Palette::border.withAlpha(alpha::scrim));
        g.drawVerticalLine((int) x, (float) gridTop(), (float) bottom);
    }
}

void PianoRollEditor::paintNotes(juce::Graphics& g) {
    const auto geo = geometry();
    const auto n = model_.editsNotes() ? model_.gestureNotes() : model_.notes();
    for (int i = 0; i < (int) n.size(); ++i) {
        const auto& e = n[(size_t) i];
        if (e.pitch > model_.topPitch || geo.pitchToY(e.pitch) >= (float) gridBottom()) continue;
        const float x = geo.tickToX(e.tick);
        const float w = juce::jmax(3.0f, geo.tickToX(e.tick + e.lengthTicks) - x - 1.0f);
        const float y = geo.pitchToY(e.pitch);
        const float bright = 0.35f + 0.65f * (float) e.velocity / kMidiMaxF;
        auto col = notecolour::fill(e.colour, Palette::accent).withMultipliedBrightness(bright);
        const bool sel = model_.selection.count(i) > 0
                      || (model_.gesture() != Gesture::None && i == model_.gestureIndex());
        if (sel) col = col.interpolatedWith(Palette::text, 0.25f);
        g.setColour(col);
        g.fillRoundedRectangle(x, y, w, (float) (kRowH - 1), 2.0f);
        if (sel) {
            g.setColour(Palette::text);
            g.drawRoundedRectangle(x, y, w, (float) (kRowH - 1), 2.0f, 1.5f);
        }
        noteedit::paintNoteName(g, {x, y, w, (float) (kRowH - 1)}, e.pitch,
                                Palette::background.withAlpha(alpha::strong));
    }

    if (const auto marquee = marqueeRect(); model_.gesture() == Gesture::Marquee && !marquee.isEmpty()) {
        g.setColour(Palette::accent.withAlpha(alpha::mist));
        g.fillRect(marquee);
        g.setColour(Palette::accent.withAlpha(alpha::strong));
        g.drawRect(marquee, 1);
    }
}

void PianoRollEditor::paintCutGuide(juce::Graphics& g) {
    if (model_.tool != Tool::Scissors || !cutGuideAt(hover_)) return;
    timelinechrome::paintCutLine(g, cutGuideX(hover_.x), (float) gridTop(), (float) gridBottom());
}

void PianoRollEditor::paintVelocity(juce::Graphics& g) {
    const auto geo = geometry();
    const int top = gridBottom(), h = kVelH;
    g.setColour(Palette::background.darker(0.15f));
    g.fillRect(0, top, getWidth(), h);
    g.setColour(Palette::border);
    g.drawHorizontalLine(top, 0.0f, (float) getWidth());

    if (model_.laneCC >= 0) { paintCCLane(g, top, h); return; }

    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("vel", 4, top + 3, kKeyW - 8, 10, juce::Justification::centredLeft, false);

    const auto ns = model_.gesture() == Gesture::None ? model_.notes() : model_.gestureNotes();
    for (size_t i = 0; i < ns.size(); ++i) {
        const float x = geo.tickToX(ns[i].tick);
        if (x < (float) gridLeft() || x > (float) getWidth()) continue;
        const float bh = (float) (h - 6) * (float) ns[i].velocity / kMidiMaxF;
        const bool sel = model_.selection.count((int) i) > 0;
        const auto col = notecolour::fill(ns[i].colour, Palette::accent);
        g.setColour(sel ? col : col.withAlpha(alpha::dim));
        g.fillRect(x, (float) (top + h - 3) - bh, 3.0f, bh);
    }
}

}
