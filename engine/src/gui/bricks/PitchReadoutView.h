// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/MidiFormat.h"
#include "gui/style/Colours.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/readouts/ScopeReadings.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class PitchReadoutView : public PolledBrick {
public:
    PitchReadoutView(BrickHost& host, std::string name)
        : PolledBrick(host, std::move(name)) {
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 336; }
    int preferredContentHeight(int) const override { return 76; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 4.0f, 1.0f);
        r.reduce(8, 6);

        if (reading_.chord().size() > 1) { paintChord(g, r); return; }

        auto noteBox = r.removeFromLeft(78);
        const bool active = reading_.note() >= 0;
        g.setColour(active ? Palette::accent : Palette::textDim.withAlpha(alpha::dim));
        g.setFont(juce::FontOptions(30.0f).withStyle("Bold"));
        g.drawText(active ? juce::String(noteName(reading_.note())) : juce::String("-"),
                   noteBox.removeFromTop(38), juce::Justification::centred);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(reading_.hz() > 0.0f ? juce::String(reading_.hz(), 1) + tr("pitch-readout.hz", " Hz") : juce::String(tr("pitch-readout.no-pitch", "no pitch")),
                   noteBox, juce::Justification::centred);

        r.removeFromLeft(8);

        auto needle = r.removeFromTop(r.getHeight() - 22);
        drawNeedle(g, needle);

        auto bars = r;
        auto lvl = bars.removeFromTop(bars.getHeight() / 2).reduced(0, 1);
        auto clr = bars.reduced(0, 1);
        drawBar(g, lvl, reading_.level(), "in", ink::state::ok);
        drawBar(g, clr, reading_.clarity(), "lock", Palette::accent);
    }

private:
    void paintChord(juce::Graphics& g, juce::Rectangle<int> r) {
        auto names = r.removeFromTop(r.getHeight() - 22);
        const auto& notes = reading_.chord();
        const int each = names.getWidth() / (int) notes.size();
        for (size_t i = 0; i < notes.size(); ++i) {
            auto cell = names.removeFromLeft(each);
            g.setColour(Palette::accent);
            g.setFont(juce::FontOptions(i == 0 ? 26.0f : 22.0f).withStyle("Bold"));
            g.drawText(juce::String(noteName(notes[i])), cell.removeFromTop(32),
                       juce::Justification::centred);
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(9.0f));
            g.drawText(i == 0 ? tr("pitch-readout.lowest", "lowest") : juce::String(),
                       cell, juce::Justification::centred);
        }
        auto bars = r;
        auto lvl = bars.removeFromTop(bars.getHeight() / 2).reduced(0, 1);
        drawBar(g, lvl, reading_.level(), "in", ink::state::ok);
        drawBar(g, bars.reduced(0, 1), reading_.clarity(), "tone", Palette::accent);
    }

    void drawNeedle(juce::Graphics& g, juce::Rectangle<int> r) {
        const float cx = r.getCentreX();
        g.setColour(Palette::textDim.withAlpha(alpha::muted));
        g.fillRect(juce::Rectangle<float>((float) r.getX(), r.getCentreY() - 0.5f,
                                          (float) r.getWidth(), 1.0f));
        g.setColour(Palette::textDim.withAlpha(alpha::mid));
        g.fillRect(juce::Rectangle<float>(cx - 0.5f, (float) r.getY() + 2.0f, 1.0f,
                                          (float) r.getHeight() - 4.0f));
        if (reading_.hz() <= 0.0f) return;
        const double cents = reading_.cents();
        const float x = cx + (float) (std::clamp(cents, -50.0, 50.0) / 50.0)
                                 * (r.getWidth() * 0.5f - 6.0f);
        const auto tuning = reading_.tuning();
        juce::Colour c = tuning == readout::Pitch::Tuning::InTune ? ink::state::ok
                       : tuning == readout::Pitch::Tuning::Close  ? ink::state::caution
                                                                   : ink::state::danger;
        g.setColour(c);
        g.fillRoundedRectangle(juce::Rectangle<float>(x - 2.0f, (float) r.getY() + 1.0f, 4.0f,
                                                      (float) r.getHeight() - 2.0f), 1.5f);
    }

    static void drawBar(juce::Graphics& g, juce::Rectangle<int> r, float v,
                        const juce::String& tag, juce::Colour col) {
        auto tagBox = r.removeFromLeft(26);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(9.0f));
        g.drawText(tag, tagBox, juce::Justification::centredLeft);
        g.setColour(Palette::background.darker(0.4f));
        g.fillRoundedRectangle(r.toFloat(), 2.0f);
        auto fill = r.toFloat().withWidth(r.getWidth() * std::clamp(v, 0.0f, 1.0f));
        g.setColour(col.withAlpha(alpha::heavy));
        g.fillRoundedRectangle(fill, 2.0f);
    }

    void poll() override {
        if (reading_.poll(host_, name_)) repaint();
    }

    readout::Pitch reading_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchReadoutView)
};

}
