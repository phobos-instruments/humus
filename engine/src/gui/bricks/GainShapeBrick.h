// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/UiTicker.h"
#include "hum/caps/Graph.h"
#include "hum/dsp/GainShape.h"
#include "gui/common/Localisation.h"

namespace hum {

class GainShapeBrick : public juce::Component, public juce::SettableTooltipClient {
public:
    GainShapeBrick(BrickHost& host, std::string organism, std::string param)
        : host_(host), cn_(std::move(organism)), pn_(std::move(param)) {
        if (!decodeGainShape(host_.liveParamText(cn_, pn_).c_str(), shape_))
            shape_ = gainShapePreset(0);
        for (int i = 0; i < kGainShapePresets; ++i)
            if (encodeGainShape(shape_) == encodeGainShape(gainShapePreset(i))) activePreset_ = i;
        tickerId_ = UiTicker::instance().add([this] { pollPlayhead(); });
    }
    ~GainShapeBrick() override { UiTicker::instance().remove(tickerId_); }

    bool playheadShowing() const { return playhead_ >= 0.0f; }
    float playheadPhase() const { return playhead_; }

    void pollPlayhead() {
        float phase = -1.0f, gain = playGain_;
        if (auto* cs = dynamic_cast<ControlSource*>(host_.liveOrganism(cn_)); cs != nullptr
                                                                             && host_.isPlaying()) {
            ControlSource::ControlVal vals[4];
            const int n = cs->controlValues(vals, 4);
            for (int i = 0; i < n; ++i) {
                if (juce::String(vals[i].name) == "phase") phase = vals[i].value;
                if (juce::String(vals[i].name) == "gain") gain = vals[i].value;
            }
        }
        const int w = juce::jmax(1, curveArea().getWidth());
        if ((int) (phase * w) == (int) (playhead_ * w) && std::abs(gain - playGain_) < 0.004f)
            return;
        playhead_ = phase;
        playGain_ = gain;
        repaint(curveArea());
    }

    void paint(juce::Graphics& g) override {
        const auto area = curveArea().toFloat();
        g.setColour(Palette::background);
        g.fillRoundedRectangle(area, 4.0f);
        const auto plot = plotArea();
        auto xOf = [&plot](double ph) { return plot.getX() + (float) ph * plot.getWidth(); };
        auto yOf = [&plot](double v) { return plot.getBottom() - (float) v * plot.getHeight(); };
        g.setColour(Palette::border.withAlpha(alpha::muted));
        for (int q = 1; q < 4; ++q)
            g.drawVerticalLine((int) xOf(q / 4.0), area.getY() + 2, area.getBottom() - 2);
        g.setFont(juce::FontOptions(10.0f));
        for (const auto& [db, label] : kAxis) {
            const float y = yOf(std::pow(10.0, db / 20.0));
            g.setColour(Palette::border.withAlpha(alpha::muted));
            if (db < 0.0) g.drawHorizontalLine((int) y, area.getX() + 2, area.getRight() - 2);
            g.setColour(Palette::textDim);
            const int ly = juce::jlimit((int) area.getY(), (int) area.getBottom() - 12, (int) y - 6);
            g.drawText(label, 0, ly, kAxisW - 4, 12, juce::Justification::centredRight);
        }
        for (int i = 0; i < kTools; ++i) {
            const auto b = toolRect(i).toFloat();
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(b, 3.0f);
            g.setColour(Palette::textDim);
            if (i >= 2) {
                g.setFont(juce::FontOptions(9.0f));
                g.drawText(i == 2 ? tr("gain-shape.rev", "REV") : tr("gain-shape.inv", "INV"), b.toNearestInt(), juce::Justification::centred);
                continue;
            }
            juce::Path arrow;
            const auto c = b.getCentre();
            const float dx = i == 0 ? -2.5f : 2.5f;
            arrow.addTriangle(c.x + dx, c.y, c.x - dx, c.y - 4.0f, c.x - dx, c.y + 4.0f);
            g.fillPath(arrow);
        }
        juce::Path curve;
        const int steps = juce::jmax(64, (int) area.getWidth() / 2);
        for (int i = 0; i <= steps; ++i) {
            const double ph = (double) i / steps;
            const double v = drawing_ ? slotValue(ph) : shape_.eval(ph);
            if (i == 0) curve.startNewSubPath(xOf(ph), yOf(v)); else curve.lineTo(xOf(ph), yOf(v));
        }
        juce::Path fill(curve);
        fill.lineTo(xOf(1.0), area.getBottom());
        fill.lineTo(xOf(0.0), area.getBottom());
        fill.closeSubPath();
        juce::Path frame;
        frame.addRoundedRectangle(area, 4.0f);
        g.saveState();
        g.reduceClipRegion(frame);
        g.setColour(Palette::accent.withAlpha(alpha::dim));
        g.fillPath(fill);
        g.setColour(Palette::accent);
        g.strokePath(curve, juce::PathStrokeType(kStroke));
        if (playhead_ >= 0.0f) {
            const float x = xOf(playhead_);
            g.setColour(Palette::text.withAlpha(alpha::mid));
            g.drawVerticalLine((int) x, area.getY() + 2, area.getBottom() - 2);
            g.setColour(Palette::text);
            g.fillEllipse(x - 3.0f, yOf(playGain_) - 3.0f, 6.0f, 6.0f);
        }
        g.restoreState();
        g.setColour(Palette::border);
        g.drawRoundedRectangle(area, 4.0f, 1.0f);

        for (int i = 0; i < kGainShapePresets; ++i) {
            const auto r = tileRect(i).toFloat();
            const bool on = i == activePreset_;
            g.setColour(on ? Palette::accent : Palette::panelLight);
            g.fillRoundedRectangle(r, 4.0f);
            juce::Path mp;
            const auto preset = gainShapePreset(i);
            const auto inner = r.reduced(5.0f, 5.0f);
            for (int k = 0; k <= 24; ++k) {
                const double ph = k / 24.0;
                const juce::Point<float> pt(inner.getX() + (float) ph * inner.getWidth(),
                                            inner.getBottom()
                                                - (float) preset.eval(ph) * inner.getHeight());
                if (k == 0) mp.startNewSubPath(pt);
                else mp.lineTo(pt);
            }
            g.setColour(on ? Palette::background : Palette::textDim);
            g.strokePath(mp, juce::PathStrokeType(1.4f));
        }
    }

    juce::Rectangle<int> curveArea() const {
        auto r = getLocalBounds();
        r.removeFromLeft(kAxisW);
        r.removeFromBottom(kTileRows * kTileH + kTileRows * kTileGap);
        return r;
    }

    void reverse() { replace(reversedGainShape(shape_)); }
    void invert() { replace(invertedGainShape(shape_)); }
    void nudge(double delta) { replace(rotatedGainShape(shape_, delta)); }

    void mouseDown(const juce::MouseEvent& e) override {
        const double step = e.mods.isShiftDown() ? kBigNudge : kNudge;
        if (toolRect(0).contains(e.getPosition())) { nudge(-step); return; }
        if (toolRect(1).contains(e.getPosition())) { nudge(step); return; }
        if (toolRect(2).contains(e.getPosition())) { reverse(); return; }
        if (toolRect(3).contains(e.getPosition())) { invert(); return; }
        for (int i = 0; i < kGainShapePresets; ++i)
            if (tileRect(i).contains(e.getPosition())) {
                shape_ = gainShapePreset(i);
                activePreset_ = i;
                push();
                repaint();
                return;
            }
        if (curveArea().contains(e.getPosition())) {
            drawing_ = true;
            for (int i = 0; i < kSlots; ++i)
                slots_[(size_t) i] = (float) shape_.eval((double) i / (kSlots - 1));
            lastSlot_ = -1;
            paintSlot(e);
        }
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (drawing_) paintSlot(e);
    }
    void mouseUp(const juce::MouseEvent&) override {
        if (!drawing_) return;
        drawing_ = false;
        simplifyIntoShape();
        activePreset_ = -1;
        push();
        repaint();
    }
    void mouseMove(const juce::MouseEvent& e) override {
        setMouseCursor(curveArea().contains(e.getPosition())
                           ? juce::MouseCursor::CrosshairCursor
                           : juce::MouseCursor::NormalCursor);
        const juce::String tip = toolRect(0).contains(e.getPosition())
                                     ? "Earlier: slide the shape back by 1/32 of the cycle (Shift: 1/8)"
                                 : toolRect(1).contains(e.getPosition())
                                     ? "Later: slide the shape on by 1/32 of the cycle (Shift: 1/8)"
                                 : toolRect(2).contains(e.getPosition())
                                     ? "Reverse: play the shape backwards in time"
                                 : toolRect(3).contains(e.getPosition())
                                     ? "Invert: flip the gain, so dips become peaks"
                                     : juce::String();
        if (tip != getTooltip()) setTooltip(tip);
    }

private:
    static constexpr int kSlots = 129;
    static constexpr int kTileRows = 2, kTileCols = 5, kTileH = 26, kTileGap = 4;

    static constexpr float kStroke = 2.0f;
    static constexpr int kAxisW = 34;
    static constexpr std::pair<double, const char*> kAxis[] = {
        {0.0, "0 dB"}, {-3.0, "-3"}, {-6.0, "-6"}, {-12.0, "-12"}, {-24.0, "-24"}};
    static constexpr double kNudge = 1.0 / 32.0;
    static constexpr double kBigNudge = 1.0 / 8.0;

    juce::Rectangle<float> plotArea() const {
        return curveArea().toFloat().reduced(kStroke, kStroke);
    }
    static constexpr int kTools = 4;

    juce::Rectangle<int> toolRect(int i) const {
        const int top = curveArea().getBottom() + kTileGap + 2;
        const int w = kAxisW - 8;
        if (i < 2) return {2 + i * (w / 2 + 1), top, w / 2 - 1, 16};
        return {2, top + (i - 1) * 19, w, 16};
    }
    void replace(const GainShape& next) {
        shape_ = next;
        activePreset_ = -1;
        for (int i = 0; i < kGainShapePresets; ++i)
            if (encodeGainShape(shape_) == encodeGainShape(gainShapePreset(i))) activePreset_ = i;
        push();
        repaint();
    }
    juce::Rectangle<int> tileRect(int i) const {
        const int row = i / kTileCols, col = i % kTileCols;
        const int span = getWidth() - kAxisW;
        const int w = (span - (kTileCols - 1) * kTileGap) / kTileCols;
        return {kAxisW + col * (w + kTileGap),
                getHeight() - (kTileRows - row) * (kTileH + kTileGap) + kTileGap, w, kTileH};
    }

    void paintSlot(const juce::MouseEvent& e) {
        const auto area = curveArea();
        const int slot = juce::jlimit(
            0, kSlots - 1,
            (int) std::lround((double) (e.x - area.getX()) / area.getWidth() * (kSlots - 1)));
        const float v = (float) juce::jlimit(
            0.0, 1.0, 1.0 - (double) (e.y - area.getY()) / area.getHeight());
        if (lastSlot_ < 0) {
            slots_[(size_t) slot] = v;
        } else {
            const int a = juce::jmin(lastSlot_, slot), b = juce::jmax(lastSlot_, slot);
            for (int i = a; i <= b; ++i) {
                const float t = b == a ? 1.0f : (float) (i - a) / (float) (b - a);
                const float from = slot >= lastSlot_ ? lastVal_ : v;
                const float to = slot >= lastSlot_ ? v : lastVal_;
                slots_[(size_t) i] = from + (to - from) * t;
            }
        }
        lastSlot_ = slot;
        lastVal_ = v;
        repaint();
    }

    double slotValue(double phase) const {
        const double f = phase * (kSlots - 1);
        const int i = juce::jlimit(0, kSlots - 2, (int) f);
        const double t = f - i;
        return slots_[(size_t) i] * (1.0 - t) + slots_[(size_t) i + 1] * t;
    }

    void simplifyIntoShape() {
        double eps = 0.008;
        for (;;) {
            GainShape g;
            int anchor = 0;
            g.add(0.0f, slots_[0]);
            while (anchor < kSlots - 1) {
                int j = anchor + 1;
                for (int cand = anchor + 2; cand < kSlots; ++cand) {
                    bool ok = true;
                    for (int k = anchor + 1; k < cand && ok; ++k) {
                        const double t = (double) (k - anchor) / (cand - anchor);
                        const double lin = slots_[(size_t) anchor]
                                         + (slots_[(size_t) cand] - slots_[(size_t) anchor]) * t;
                        ok = std::abs(lin - slots_[(size_t) k]) <= eps;
                    }
                    if (!ok) break;
                    j = cand;
                }
                if (g.n >= GainShape::kMaxPoints) break;
                g.add((float) j / (kSlots - 1), slots_[(size_t) j]);
                anchor = j;
            }
            if (anchor >= kSlots - 1 && g.n <= GainShape::kMaxPoints) {
                shape_ = g;
                return;
            }
            eps *= 1.7;
        }
    }

    void push() { host_.setParamText(cn_, pn_, encodeGainShape(shape_)); }

    BrickHost& host_;
    std::string cn_, pn_;
    GainShape shape_;
    int activePreset_ = -1;
    bool drawing_ = false;
    float slots_[kSlots] = {};
    int lastSlot_ = -1;
    float lastVal_ = 0.0f;
    int tickerId_ = 0;
    float playhead_ = -1.0f;
    float playGain_ = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainShapeBrick)
};

}
