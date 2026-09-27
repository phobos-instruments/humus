// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/editor/readouts/ScopeTrace.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"
#include "hum/dsp/DspMath.h"

namespace hum {

class TraceScopeView : public PolledBrick {
public:
    static constexpr int kDivisionsAcross = 10, kDivisionsUp = 8;
    static constexpr float kPersistence = 0.72f;
    static constexpr int kLissajousSamples = 4096;

    TraceScopeView(BrickHost& host, std::string name) : PolledBrick(host, std::move(name)) {}

    int preferredContentWidth() const override { return 269; }
    int preferredContentHeight(int) const override { return 190; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(ink::spectrum::ground);
        g.fillRoundedRectangle(r, 6.0f);
        drawGraticule(g, screen());
        if (glow_.isValid())
            g.drawImage(glow_, screen().toFloat(), juce::RectanglePlacement::stretchToFit);
        g.setColour(ink::spectrum::frame);
        g.drawRoundedRectangle(r, 6.0f, 1.2f);
    }

    void primePreview() {
        TraceSource::Shape shape;
        shape.windowSamples = 960;
        std::vector<float> l(960), r(960);
        for (int i = 0; i < 960; ++i) {
            const double t = (double) i / 960.0;
            l[(size_t) i] = (float) (0.62 * std::sin(kTwoPi * 3.0 * t) + 0.2 * std::sin(kTwoPi * 9.0 * t));
            r[(size_t) i] = (float) (0.5 * std::sin(kTwoPi * 3.0 * t + 0.6));
        }
        reading_.load(shape, std::move(l), std::move(r));
        for (int pass = 0; pass < 3; ++pass) expose();
        repaint();
    }

    juce::Image glowForTest() const { return glow_; }

private:
    juce::Rectangle<int> screen() const { return getLocalBounds().reduced(7); }

    static void drawGraticule(juce::Graphics& g, juce::Rectangle<int> area) {
        g.setColour(ink::spectrum::grid);
        for (int i = 0; i <= kDivisionsAcross; ++i) {
            const int x = area.getX() + area.getWidth() * i / kDivisionsAcross;
            g.drawVerticalLine(std::min(x, area.getRight() - 1), (float) area.getY(), (float) area.getBottom());
        }
        for (int i = 0; i <= kDivisionsUp; ++i) {
            const int y = area.getY() + area.getHeight() * i / kDivisionsUp;
            g.drawHorizontalLine(std::min(y, area.getBottom() - 1), (float) area.getX(), (float) area.getRight());
        }
        g.setColour(ink::spectrum::label);
        const int midX = area.getCentreX(), midY = area.getCentreY();
        for (int i = 0; i <= kDivisionsAcross * 5; ++i) {
            const float x = (float) area.getX() + (float) area.getWidth() * (float) i / (float) (kDivisionsAcross * 5);
            g.drawVerticalLine((int) x, (float) midY - 2.0f, (float) midY + 2.0f);
        }
        for (int i = 0; i <= kDivisionsUp * 5; ++i) {
            const float y = (float) area.getY() + (float) area.getHeight() * (float) i / (float) (kDivisionsUp * 5);
            g.drawHorizontalLine((int) y, (float) midX - 2.0f, (float) midX + 2.0f);
        }
    }

    void poll() override {
        if (!reading_.poll(host_, name_)) return;
        expose();
        repaint();
    }

    void expose() {
        const auto area = screen();
        const int w = std::max(2, area.getWidth() * 2), h = std::max(2, area.getHeight() * 2);
        if (!glow_.isValid() || glow_.getWidth() != w || glow_.getHeight() != h)
            glow_ = juce::Image(juce::Image::ARGB, w, h, true);
        glow_.multiplyAllAlphas(kPersistence);
        if (reading_.fading()) return;
        juce::Graphics g(glow_);
        const auto& shape = reading_.shape();
        if (shape.plotsLeftAgainstRight) {
            sweepFigure(g, w, h, shape.gain);
            return;
        }
        sweepWave(g, reading_.left(), w, h, shape.gain, ink::spectrum::trace);
        if (!shape.sumsToMono) sweepWave(g, reading_.right(), w, h, shape.gain, ink::spectrum::hold);
    }

    static void beam(juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, juce::Colour ink) {
        const float bright = readout::beamBrightness(a.getDistanceFrom(b) / 6.0f);
        g.setColour(ink.withAlpha(0.22f * bright));
        g.drawLine({a, b}, 5.0f);
        g.setColour(ink.interpolatedWith(ink::spectrum::heatPeak, 0.35f * bright).withAlpha(0.35f + 0.6f * bright));
        g.drawLine({a, b}, 1.6f);
    }

    static float yOf(float sample, float gain, int h) {
        const float v = std::clamp(sample * gain, -1.2f, 1.2f);
        return (float) h * 0.5f * (1.0f - v);
    }

    void sweepWave(juce::Graphics& g, const std::vector<float>& wave, int w, int h, float gain, juce::Colour ink) {
        const int count = (int) wave.size();
        if (count < 2) return;
        if (count <= w) {
            juce::Point<float> last(0.0f, yOf(wave[0], gain, h));
            for (int i = 1; i < count; ++i) {
                const juce::Point<float> next((float) w * (float) i / (float) (count - 1), yOf(wave[(size_t) i], gain, h));
                beam(g, last, next, ink);
                last = next;
            }
            return;
        }
        readout::traceColumns(wave.data(), count, w / 2, columns_);
        juce::Point<float> last(0.0f, yOf(columns_.front().high, gain, h));
        for (size_t c = 0; c < columns_.size(); ++c) {
            const float x = (float) c * 2.0f + 1.0f;
            juce::Point<float> top(x, yOf(columns_[c].high, gain, h)), bottom(x, yOf(columns_[c].low, gain, h));
            if (std::abs(last.y - bottom.y) < std::abs(last.y - top.y)) std::swap(top, bottom);
            beam(g, last, top, ink);
            if (std::abs(top.y - bottom.y) >= 0.5f) beam(g, top, bottom, ink);
            last = bottom;
        }
    }

    void sweepFigure(juce::Graphics& g, int w, int h, float gain) {
        const auto& l = reading_.left();
        const auto& r = reading_.right();
        const int count = (int) std::min(l.size(), r.size());
        const int from = std::max(0, count - kLissajousSamples);
        const float side = (float) std::min(w, h);
        const auto spot = [&](int i) {
            const float x = std::clamp(l[(size_t) i] * gain, -1.2f, 1.2f);
            const float y = std::clamp(r[(size_t) i] * gain, -1.2f, 1.2f);
            return juce::Point<float>((float) w * 0.5f + x * side * 0.5f, (float) h * 0.5f - y * side * 0.5f);
        };
        for (int i = from + 1; i < count; ++i) beam(g, spot(i - 1), spot(i), ink::spectrum::trace);
    }

    readout::Trace reading_;
    std::vector<readout::TraceColumn> columns_;
    juce::Image glow_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TraceScopeView)
};

}
