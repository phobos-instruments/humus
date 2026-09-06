#pragma once
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/PolledBrick.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "hum/Capabilities.h"
#include "gui/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class PitchReadoutView : public PolledBrick {
public:
    PitchReadoutView(EngineHost& host, std::string name)
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

        auto noteBox = r.removeFromLeft(78);
        const bool active = note_ >= 0;
        g.setColour(active ? Palette::accent : Palette::textDim.withAlpha(0.5f));
        g.setFont(juce::FontOptions(30.0f).withStyle("Bold"));
        g.drawText(active ? noteName(note_) : juce::String("-"),
                   noteBox.removeFromTop(38), juce::Justification::centred);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(hz_ > 0.0f ? juce::String(hz_, 1) + tr("pitch-readout.hz", " Hz") : juce::String(tr("pitch-readout.no-pitch", "no pitch")),
                   noteBox, juce::Justification::centred);

        r.removeFromLeft(8);

        auto needle = r.removeFromTop(r.getHeight() - 22);
        drawNeedle(g, needle);

        auto bars = r;
        auto lvl = bars.removeFromTop(bars.getHeight() / 2).reduced(0, 1);
        auto clr = bars.reduced(0, 1);
        drawBar(g, lvl, level_, "in", juce::Colour(0xff37c86a));
        drawBar(g, clr, clarity_, "lock", Palette::accent);
    }

private:
    static juce::String noteName(int midi) {
        static const char* n[12] = {"C", "C#", "D", "D#", "E", "F",
                                    "F#", "G", "G#", "A", "A#", "B"};
        return juce::String(n[(midi % 12 + 12) % 12]) + juce::String(midi / 12 - 1);
    }

    void drawNeedle(juce::Graphics& g, juce::Rectangle<int> r) {
        const float cx = r.getCentreX();
        g.setColour(Palette::textDim.withAlpha(0.35f));
        g.fillRect(juce::Rectangle<float>((float) r.getX(), r.getCentreY() - 0.5f,
                                          (float) r.getWidth(), 1.0f));
        g.setColour(Palette::textDim.withAlpha(0.6f));
        g.fillRect(juce::Rectangle<float>(cx - 0.5f, (float) r.getY() + 2.0f, 1.0f,
                                          (float) r.getHeight() - 4.0f));
        if (hz_ <= 0.0f) return;
        const double midi = hzToMidi((double) hz_);
        const double cents = (midi - std::round(midi)) * 100.0;
        const float x = cx + (float) (std::clamp(cents, -50.0, 50.0) / 50.0)
                                 * (r.getWidth() * 0.5f - 6.0f);
        const double ac = std::abs(cents);
        juce::Colour c = ac < 5.0 ? juce::Colour(0xff37c86a)
                       : ac < 20.0 ? juce::Colour(0xffe6c83c)
                                   : juce::Colour(0xffe1463c);
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
        g.setColour(col.withAlpha(0.85f));
        g.fillRoundedRectangle(fill, 2.0f);
    }

    void poll() override {
        auto* src = dynamic_cast<PitchDetectSource*>(host_.liveOrganism(name_));
        const float hz = src ? src->detectedHz() : 0.0f;
        const float lv = src ? src->detectLevel() : 0.0f;
        const float cl = src ? src->detectClarity() : 0.0f;
        const int nt = src ? src->detectedNote() : -1;
        if (std::abs(hz - hz_) > 0.2f || std::abs(lv - level_) > 0.01f
            || std::abs(cl - clarity_) > 0.01f || nt != note_) {
            hz_ = hz; level_ = lv; clarity_ = cl; note_ = nt;
            repaint();
        }
    }

    float hz_ = 0.0f, level_ = 0.0f, clarity_ = 0.0f;
    int note_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchReadoutView)
};

}
