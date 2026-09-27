// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/readouts/SpectrumPreview.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/readouts/SpectrumReading.h"

namespace hum {

class SpectrumScopeView : public PolledBrick {
public:
    SpectrumScopeView(BrickHost& host, std::string name)
        : PolledBrick(host, std::move(name)), fft_(Reading::kOrder) {}

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 576; }
    int preferredContentHeight(int) const override { return 220; }

    static juce::Colour ground()  { return ink::spectrum::ground; }
    static juce::Colour gridInk() { return ink::spectrum::grid; }
    static juce::Colour labelInk(){ return ink::spectrum::label; }
    static juce::Colour trace()   { return ink::spectrum::trace; }
    static juce::Colour holdInk() { return ink::spectrum::hold; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(ground());
        g.fillRoundedRectangle(r, 6.0f);

        auto area = getLocalBounds().reduced(6);
        auto curveArea = area.removeFromTop(area.getHeight() * 2 / 5);
        area.removeFromTop(3);
        auto fallArea = area;

        g.setColour(gridInk());
        for (int db = -60; db < 0; db += 24)
            g.drawHorizontalLine(yOfDb(curveArea, (float) db),
                                 (float) curveArea.getX(), (float) curveArea.getRight());
        g.setFont(juce::FontOptions(8.0f));
        for (double hz : {100.0, 1000.0, 10000.0}) {
            const int x = curveArea.getX()
                + (int) std::lround(xOfHz(hz) * (double) curveArea.getWidth());
            g.setColour(gridInk());
            g.drawVerticalLine(x, (float) curveArea.getY(), (float) fallArea.getBottom());
            g.setColour(labelInk());
            g.drawText(hz >= 1000.0 ? juce::String(hz / 1000.0, 0) + "k"
                                    : juce::String((int) hz),
                       x + 2, fallArea.getBottom() - 11, 24, 10,
                       juce::Justification::left);
        }

        {
            juce::Path fill;
            fill.startNewSubPath((float) curveArea.getX(), (float) curveArea.getBottom());
            for (int c = 0; c < kCols; ++c)
                fill.lineTo(xOfCol(curveArea, c), (float) yOfDb(curveArea, reading_.column(c)));
            fill.lineTo((float) curveArea.getRight(), (float) curveArea.getBottom());
            fill.closeSubPath();
            g.setColour(trace().withAlpha(alpha::scrim));
            g.fillPath(fill);
            juce::Path line;
            for (int c = 0; c < kCols; ++c) {
                const juce::Point<float> p(xOfCol(curveArea, c),
                                           (float) yOfDb(curveArea, reading_.column(c)));
                if (c == 0) line.startNewSubPath(p); else line.lineTo(p);
            }
            g.setColour(trace());
            g.strokePath(line, juce::PathStrokeType(1.4f));
            juce::Path hold;
            for (int c = 0; c < kCols; ++c) {
                const juce::Point<float> p(xOfCol(curveArea, c),
                                           (float) yOfDb(curveArea, reading_.peak(c)));
                if (c == 0) hold.startNewSubPath(p); else hold.lineTo(p);
            }
            g.setColour(holdInk().withAlpha(alpha::mid));
            g.strokePath(hold, juce::PathStrokeType(0.8f));
        }

        if (fall_.isValid()) {
            g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
            const int head = reading_.head();
            const int newest = kRows - head;
            g.drawImage(fall_, fallArea.getX(), fallArea.getY(),
                        fallArea.getWidth(), fallArea.getHeight() * newest / kRows,
                        0, head, kCols, newest);
            if (head > 0)
                g.drawImage(fall_, fallArea.getX(),
                            fallArea.getY() + fallArea.getHeight() * newest / kRows,
                            fallArea.getWidth(),
                            fallArea.getHeight() - fallArea.getHeight() * newest / kRows,
                            0, 0, kCols, head);
        }

        g.setColour(ink::spectrum::frame);
        g.drawRoundedRectangle(r, 6.0f, 1.2f);
    }

private:
    using Reading = readout::Spectrum;
    static constexpr int kCols = Reading::kCols, kRows = Reading::kRows;

    static double xOfHz(double hz) { return Reading::xOfHz(hz); }
    static float xOfCol(juce::Rectangle<int> a, int c) {
        return (float) a.getX() + (float) a.getWidth() * (float) c / (float) (kCols - 1);
    }
    static int yOfDb(juce::Rectangle<int> a, float db) {
        const float t = Reading::heightOfDb(db);
        return a.getBottom() - (int) std::lround(t * (float) a.getHeight());
    }
    static juce::Colour heat(float t) {
        t = juce::jlimit(0.0f, 1.0f, t);
        const auto sea = ink::spectrum::heatSea, mint = ink::spectrum::heatMint,
                   hot = ink::spectrum::heatPeak;
        if (t < 0.40f) return ground().interpolatedWith(sea, t / 0.40f);
        if (t < 0.75f) return sea.interpolatedWith(trace(), (t - 0.40f) / 0.35f);
        if (t < 0.92f) return trace().interpolatedWith(mint, (t - 0.75f) / 0.17f);
        return mint.interpolatedWith(hot, (t - 0.92f) / 0.08f);
    }

    void poll() override {
        float frame[Reading::kFftSize];
        double rate = 0.0;
        if (!reading_.poll(host_, name_, frame, rate)) return;
        analyze(frame, rate);
        repaint();
    }

public:
    void primePreview() {
        spectrum::PreviewMix mix(kDefaultSampleRate);
        for (int f = 0; f < Reading::previewFrames(); ++f) {
            float frame[Reading::kFftSize];
            mix.fill(frame, Reading::kFftSize);
            analyze(frame, kDefaultSampleRate);
        }
        repaint();
    }

private:
    void analyze(const float* samples, double sr) {
        reading_.analyze(samples, sr, [this](float* buf) { fft_.performFrequencyOnlyForwardTransform(buf); });
        if (!fall_.isValid()) fall_ = juce::Image(juce::Image::RGB, kCols, kRows, true);
        juce::Image::BitmapData rows(fall_, juce::Image::BitmapData::writeOnly);
        for (int c = 0; c < kCols; ++c) rows.setPixelColour(c, reading_.head(), heat(reading_.heat(c)));
    }

    juce::dsp::FFT fft_;
    Reading reading_;
    juce::Image fall_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumScopeView)
};

}
