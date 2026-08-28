#pragma once
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "hum/dsp/GainShape.h"

namespace hum {

class GainShapeBrick : public juce::Component {
public:
    GainShapeBrick(EngineHost& host, std::string organism, std::string param)
        : host_(host), cn_(std::move(organism)), pn_(std::move(param)) {
        if (!decodeGainShape(host_.liveParamText(cn_, pn_).c_str(), shape_))
            shape_ = gainShapePreset(0);
        for (int i = 0; i < kGainShapePresets; ++i)
            if (encodeGainShape(shape_) == encodeGainShape(gainShapePreset(i))) activePreset_ = i;
    }

    void paint(juce::Graphics& g) override {
        const auto area = curveArea().toFloat();
        g.setColour(Palette::background);
        g.fillRoundedRectangle(area, 4.0f);
        g.setColour(Palette::border.withAlpha(0.35f));
        for (int q = 1; q < 4; ++q) {
            const float x = area.getX() + area.getWidth() * (float) q / 4.0f;
            const float y = area.getY() + area.getHeight() * (float) q / 4.0f;
            g.drawVerticalLine((int) x, area.getY() + 2, area.getBottom() - 2);
            g.drawHorizontalLine((int) y, area.getX() + 2, area.getRight() - 2);
        }
        juce::Path p;
        p.startNewSubPath(area.getX(), area.getBottom());
        const int steps = juce::jmax(64, (int) area.getWidth() / 2);
        for (int i = 0; i <= steps; ++i) {
            const double ph = (double) i / steps;
            const double v = drawing_ ? slotValue(ph) : shape_.eval(ph);
            p.lineTo(area.getX() + (float) ph * area.getWidth(),
                     area.getBottom() - (float) v * area.getHeight());
        }
        p.lineTo(area.getRight(), area.getBottom());
        p.closeSubPath();
        g.setColour(Palette::accent.withAlpha(0.45f));
        g.fillPath(p);
        g.setColour(Palette::accent);
        g.strokePath(p, juce::PathStrokeType(2.0f));
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

    void mouseDown(const juce::MouseEvent& e) override {
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
    }

private:
    static constexpr int kSlots = 129;
    static constexpr int kTileRows = 2, kTileCols = 5, kTileH = 26, kTileGap = 4;

    juce::Rectangle<int> curveArea() const {
        auto r = getLocalBounds();
        r.removeFromBottom(kTileRows * kTileH + kTileRows * kTileGap);
        return r;
    }
    juce::Rectangle<int> tileRect(int i) const {
        const int row = i / kTileCols, col = i % kTileCols;
        const int w = (getWidth() - (kTileCols - 1) * kTileGap) / kTileCols;
        return {col * (w + kTileGap),
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

    EngineHost& host_;
    std::string cn_, pn_;
    GainShape shape_;
    int activePreset_ = -1;
    bool drawing_ = false;
    float slots_[kSlots] = {};
    int lastSlot_ = -1;
    float lastVal_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainShapeBrick)
};

}
