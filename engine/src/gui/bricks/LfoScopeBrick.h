// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "io/PatchDocument.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/readouts/ScopeReadings.h"
#include "core/packs/Categories.h"
#include "hum/caps/Graph.h"

namespace hum {

class LfoScopeView : public PolledBrick {
public:
    LfoScopeView(BrickHost& host, std::string name, const Bindings& bound)
        : PolledBrick(host, std::move(name)),
          reading_(bound(bind::kWaveform), bound(bind::kAmplitude), bound(bind::kOffset)) {}

    int preferredContentWidth() const override { return 277; }
    bool tracedForTest() const { return kind_ == MotionTrace::Kind::Trace; }
    bool cycledForTest() const { return kind_ == MotionTrace::Kind::Cycle; }
    int pointsForTest() const { return shown_; }
    int preferredContentHeight(int) const override { return 56; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::background);
        g.fillRoundedRectangle(r, 4.0f);

        auto toY = [&r](double v) {
            return r.getCentreY() - (float) juce::jlimit(-1.25, 1.25, v) * r.getHeight() * 0.4f;
        };

        g.setColour(Palette::border.withAlpha(alpha::dim));
        g.drawHorizontalLine((int) toY(0.0), r.getX() + 2.0f, r.getRight() - 2.0f);
        if (kind_ == MotionTrace::Kind::Trace) {
            paintTrace(g, r, toY);
            return;
        }

        const auto shape = reading_.shape(host_, name_);
        if (kind_ == MotionTrace::Kind::Cycle) {
            paintCycle(g, r, toY, shape);
            return;
        }

        juce::Path p;
        const int steps = juce::jmax(2, (int) r.getWidth());
        for (int i = 0; i <= steps; ++i) {
            const double ph = (double) i / steps;
            const float x = r.getX() + (float) i / steps * r.getWidth();
            const float y = toY(reading_.valueAt(shape, ph));
            if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
        }
        const auto ink = familyColour();
        g.setColour(ink.withAlpha(alpha::heavy));
        g.strokePath(p, juce::PathStrokeType(1.6f));

        const float x = r.getX() + reading_.phase() * r.getWidth();
        g.setColour(Palette::textDim.withAlpha(alpha::dim));
        g.drawVerticalLine((int) x, r.getY() + 2.0f, r.getBottom() - 2.0f);
        g.setColour(ink);
        g.fillEllipse(x - 2.5f, toY(reading_.valueAt(shape, reading_.phase())) - 2.5f, 5.0f, 5.0f);
    }

protected:
    void poll() override {
        auto* motion = live<MotionTrace>();
        const auto kind = motion != nullptr ? motion->motionKind() : MotionTrace::Kind::Params;
        if (kind == MotionTrace::Kind::Trace) {
            shown_ = motion->motionRead(points_.data(), MotionTrace::kPoints);
            kind_ = kind;
            repaint();
            return;
        }
        const bool changed = kind_ != kind;
        kind_ = kind;
        if (kind == MotionTrace::Kind::Cycle) shown_ = motion->motionRead(points_.data(), MotionTrace::kPoints);
        if (reading_.poll(host_, name_, getWidth()) || changed || kind == MotionTrace::Kind::Cycle) repaint();
    }

private:
    template <class ToY>
    void paintCycle(juce::Graphics& g, juce::Rectangle<float> r, ToY&& toY, const readout::LfoCycle::Shape& shape) const {
        if (shown_ <= 1) return;
        auto at = [&](double ph) {
            const double x = std::clamp(ph, 0.0, 1.0) * (shown_ - 1);
            const int i0 = std::min((int) x, shown_ - 2);
            const double f = x - i0;
            const double v = points_[(size_t) i0].high + f * (points_[(size_t) i0 + 1].high - points_[(size_t) i0].high);
            return shape.offset + shape.amplitude * v;
        };
        juce::Path p;
        const int steps = juce::jmax(2, (int) r.getWidth());
        for (int i = 0; i <= steps; ++i) {
            const double ph = (double) i / steps;
            const float x = r.getX() + (float) ph * r.getWidth();
            if (i == 0) p.startNewSubPath(x, toY(at(ph))); else p.lineTo(x, toY(at(ph)));
        }
        const auto ink = familyColour();
        g.setColour(ink.withAlpha(alpha::heavy));
        g.strokePath(p, juce::PathStrokeType(1.6f));
        const float x = r.getX() + reading_.phase() * r.getWidth();
        g.setColour(Palette::textDim.withAlpha(alpha::dim));
        g.drawVerticalLine((int) x, r.getY() + 2.0f, r.getBottom() - 2.0f);
        g.setColour(ink);
        g.fillEllipse(x - 2.5f, toY(at(reading_.phase())) - 2.5f, 5.0f, 5.0f);
    }

    template <class ToY>
    void paintTrace(juce::Graphics& g, juce::Rectangle<float> r, ToY&& toY) const {
        if (shown_ <= 1) return;
        const float step = r.getWidth() / (float) (MotionTrace::kPoints - 1);
        const float left = r.getRight() - step * (float) (shown_ - 1);
        juce::Path band, line;
        for (int i = 0; i < shown_; ++i) {
            const auto& pt = points_[(size_t) i];
            const float x = left + step * (float) i;
            if (i == 0) band.startNewSubPath(x, toY(pt.high));
            else band.lineTo(x, toY(pt.high));
            const float mid = toY(0.5 * (pt.low + pt.high));
            if (i == 0) line.startNewSubPath(x, mid);
            else line.lineTo(x, mid);
        }
        for (int i = shown_ - 1; i >= 0; --i) band.lineTo(left + step * (float) i, toY(points_[(size_t) i].low));
        band.closeSubPath();
        const auto ink = familyColour();
        g.setColour(ink.withAlpha(alpha::dim));
        g.fillPath(band);
        g.setColour(ink.withAlpha(alpha::heavy));
        g.strokePath(line, juce::PathStrokeType(1.6f));
    }

    readout::LfoCycle reading_;
    std::array<MotionTrace::Point, MotionTrace::kPoints> points_{};
    int shown_ = 0;
    MotionTrace::Kind kind_ = MotionTrace::Kind::Params;

    juce::Colour familyColour() const {
        const auto* cm = host_.model().byName(name_);
        return Palette::familyAccent(cm ? familyOf(cm->displayClass) : Family::Motion);
    }
};

}
