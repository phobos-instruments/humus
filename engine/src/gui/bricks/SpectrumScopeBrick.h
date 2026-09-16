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
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "hum/caps/Audio.h"
#include "hum/dsp/DspMath.h"

namespace hum {

class SpectrumScopeView : public PolledBrick {
public:
    SpectrumScopeView(BrickHost& host, std::string name)
        : PolledBrick(host, std::move(name)), fft_(kOrder) {
        for (int i = 0; i < kFftSize; ++i)
            window_[(size_t) i] = 0.5f * (1.0f - std::cos(
                juce::MathConstants<float>::twoPi * (float) i / (float) (kFftSize - 1)));
        cols_.resize(kCols, kFloorDb);
        peaks_.resize(kCols, kFloorDb);
    }

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
                fill.lineTo(xOfCol(curveArea, c), (float) yOfDb(curveArea, cols_[(size_t) c]));
            fill.lineTo((float) curveArea.getRight(), (float) curveArea.getBottom());
            fill.closeSubPath();
            g.setColour(trace().withAlpha(alpha::scrim));
            g.fillPath(fill);
            juce::Path line;
            for (int c = 0; c < kCols; ++c) {
                const juce::Point<float> p(xOfCol(curveArea, c),
                                           (float) yOfDb(curveArea, cols_[(size_t) c]));
                if (c == 0) line.startNewSubPath(p); else line.lineTo(p);
            }
            g.setColour(trace());
            g.strokePath(line, juce::PathStrokeType(1.4f));
            juce::Path hold;
            for (int c = 0; c < kCols; ++c) {
                const juce::Point<float> p(xOfCol(curveArea, c),
                                           (float) yOfDb(curveArea, peaks_[(size_t) c]));
                if (c == 0) hold.startNewSubPath(p); else hold.lineTo(p);
            }
            g.setColour(holdInk().withAlpha(alpha::mid));
            g.strokePath(hold, juce::PathStrokeType(0.8f));
        }

        if (fall_.isValid()) {
            g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
            const int newest = kRows - head_;
            g.drawImage(fall_, fallArea.getX(), fallArea.getY(),
                        fallArea.getWidth(), fallArea.getHeight() * newest / kRows,
                        0, head_, kCols, newest);
            if (head_ > 0)
                g.drawImage(fall_, fallArea.getX(),
                            fallArea.getY() + fallArea.getHeight() * newest / kRows,
                            fallArea.getWidth(),
                            fallArea.getHeight() - fallArea.getHeight() * newest / kRows,
                            0, 0, kCols, head_);
        }

        g.setColour(ink::spectrum::frame);
        g.drawRoundedRectangle(r, 6.0f, 1.2f);
    }

private:
    static constexpr int kOrder = 11, kFftSize = 1 << kOrder;
    static constexpr int kCols = 192, kRows = 128;
    static constexpr float kFloorDb = -72.0f, kLoHz = 30.0f, kHiHz = 18000.0f;

    static double xOfHz(double hz) {
        return std::log(hz / kLoHz) / std::log((double) kHiHz / kLoHz);
    }
    static float xOfCol(juce::Rectangle<int> a, int c) {
        return (float) a.getX() + (float) a.getWidth() * (float) c / (float) (kCols - 1);
    }
    static int yOfDb(juce::Rectangle<int> a, float db) {
        const float t = juce::jlimit(0.0f, 1.0f, (db - kFloorDb) / -kFloorDb);
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
        auto* src = dynamic_cast<ScopeSource*>(host_.liveOrganism(name_));
        if (src == nullptr) return;
        const unsigned stamp = src->scopeStamp();
        if (stamp == lastStamp_) return;
        lastStamp_ = stamp;

        float frame[kFftSize] = {};
        src->scopeRead(frame, kFftSize);
        analyze(frame, src->scopeRate());
        repaint();
    }

public:
    void primePreview() {
        const double sr = kDefaultSampleRate;
        const double pi2 = 2.0 * kPi;
        std::uint32_t rng = 0xC0FFEEu;
        for (int f = 0; f < kRows - 16; ++f) {
            float frame[kFftSize];
            const double sweep = 300.0 * std::pow(40.0, (double) f / (double) (kRows - 16));
            for (int i = 0; i < kFftSize; ++i) {
                const double t = (double) (f * kFftSize + i) / sr;
                double v = 0.5 * std::sin(pi2 * 55.0 * t)
                         + 0.22 * std::sin(pi2 * 220.0 * t)
                         + 0.18 * std::sin(pi2 * 261.63 * t)
                         + 0.16 * std::sin(pi2 * 329.63 * t)
                         + 0.25 * std::sin(pi2 * sweep * t);
                rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                v += 0.02 * ((double) (rng & 0xFFFFFF) / (double) 0x7FFFFF - 1.0);
                frame[i] = (float) v;
            }
            analyze(frame, sr);
        }
        repaint();
    }

private:
    void analyze(const float* samples, double sr) {
        float buf[2 * kFftSize] = {};
        for (int i = 0; i < kFftSize; ++i) buf[i] = samples[i] * window_[(size_t) i];
        fft_.performFrequencyOnlyForwardTransform(buf);
        const double hzPerBin = sr / (double) kFftSize;

        if (!fall_.isValid()) fall_ = juce::Image(juce::Image::RGB, kCols, kRows, true);
        juce::Image::BitmapData rows(fall_, juce::Image::BitmapData::writeOnly);
        head_ = (head_ + kRows - 1) % kRows;

        for (int c = 0; c < kCols; ++c) {
            const double f0 = kLoHz * std::pow((double) kHiHz / kLoHz,
                                               (double) c / (double) kCols);
            const double f1 = kLoHz * std::pow((double) kHiHz / kLoHz,
                                               (double) (c + 1) / (double) kCols);
            int b0 = juce::jmax(1, (int) (f0 / hzPerBin));
            int b1 = juce::jmax(b0 + 1, (int) std::ceil(f1 / hzPerBin));
            b1 = juce::jmin(b1, kFftSize / 2);
            float mag = 0.0f;
            for (int b = b0; b < b1; ++b) mag = juce::jmax(mag, buf[b]);
            const float db = juce::jlimit(kFloorDb, 0.0f,
                20.0f * std::log10(juce::jmax(1.0e-9f, mag * 2.0f / (float) kFftSize)));
            auto& col = cols_[(size_t) c];
            col = db > col ? db : col - 2.4f;
            if (col < kFloorDb) col = kFloorDb;
            auto& pk = peaks_[(size_t) c];
            pk = juce::jmax(db, pk - 0.35f);
            if (pk < kFloorDb) pk = kFloorDb;
            rows.setPixelColour(c, head_, heat((db - kFloorDb) / -kFloorDb));
        }
    }

    juce::dsp::FFT fft_;
    float window_[kFftSize];
    std::vector<float> cols_, peaks_;
    juce::Image fall_;
    int head_ = 0;
    unsigned lastStamp_ = ~0u;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumScopeView)
};

}
