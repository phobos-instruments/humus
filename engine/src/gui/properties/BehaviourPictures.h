// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/net/ControlShape.h"
#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

#include "hum/dsp/DspMath.h"

namespace hum {

struct PressStep {
    double press = 0.0;
    double value = 0.0;
    bool acted = false;
};

inline constexpr std::array<int, 5> kExamplePresses{100, 40, 110, 90, 120};
inline constexpr double kExampleStart = 0.3;
inline constexpr double kExampleDefault = 0.5;

inline std::vector<PressStep> simulatePresses(const ControlShape& shape) {
    ControlShape s = shape;
    s.type = ControlType::Button;
    ControlShapeState st;
    double v = kExampleStart;
    std::vector<PressStep> out;
    auto feed = [&](double t, double press) {
        const auto a = advanceControl(s, st, t, 0.0);
        const bool acted = !a.none();
        if (a.kind == ControlActionKind::Absolute) v = a.value;
        else if (a.kind == ControlActionKind::Delta) v = std::clamp(v + a.value, 0.0, 1.0);
        else if (a.kind == ControlActionKind::Reset) v = kExampleDefault;
        out.push_back({press, v, acted});
    };
    feed(0.0, 0.0);
    for (int vel : kExamplePresses) {
        feed(vel / kMidiMaxD, vel / kMidiMaxD);
        feed(0.0, 0.0);
    }
    return out;
}

class PressTimeline : public juce::Component {
public:
    void setShape(const ControlShape& s) { shape_ = s; steps_ = simulatePresses(s); repaint(); }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(4.0f);
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, 4.0f);
        r = r.reduced(12.0f, 10.0f);
        auto top = r.removeFromTop(r.getHeight() * 0.45f);
        r.removeFromTop(14.0f);
        auto bottom = r;
        label(g, top, tr("mapping-shape.you-press", "you press"));
        label(g, bottom, tr("mapping-shape.parameter-does", "the parameter"));
        top.removeFromTop(14.0f);
        bottom.removeFromTop(14.0f);
        const float slot = top.getWidth() / (float) steps_.size();
        const float thresholdY = top.getBottom() - (float) shape_.threshold * top.getHeight();
        const float dashes[] = {3.0f, 3.0f};
        g.setColour(Palette::textDim.withAlpha(alpha::strong));
        g.drawDashedLine({top.getX(), thresholdY, top.getRight(), thresholdY}, dashes, 2, 1.0f);
        juce::Path level;
        for (size_t i = 0; i < steps_.size(); ++i) {
            const auto& st = steps_[i];
            const float x = top.getX() + slot * (float) i;
            if (st.press > 0.0) {
                const bool counts = (st.press > shape_.threshold) != shape_.inverted;
                const float h = (float) st.press * top.getHeight();
                g.setColour(counts ? Palette::accent : Palette::textDim.withAlpha(alpha::muted));
                g.fillRoundedRectangle(x + slot * 0.2f, top.getBottom() - h, slot * 0.6f, h, 2.0f);
                if (!counts) {
                    g.setColour(Palette::textDim);
                    g.setFont(juce::FontOptions(10.0f));
                    g.drawText(tr("mapping-shape.too-soft", "too soft"),
                               juce::Rectangle<float>(x - 6.0f, top.getBottom() - h - 14.0f, slot + 12.0f, 12.0f),
                               juce::Justification::centred);
                }
            }
            const float y = bottom.getBottom() - (float) st.value * bottom.getHeight();
            if (i == 0) level.startNewSubPath(x, y);
            else level.lineTo(x, level.getCurrentPosition().y);
            level.lineTo(x, y);
            level.lineTo(x + slot, y);
            if (st.acted && st.press > 0.0) {
                juce::Path mark;
                const float cx = x + slot * 0.5f, by = bottom.getBottom() + 4.0f;
                mark.addTriangle(cx - 4.0f, by + 6.0f, cx + 4.0f, by + 6.0f, cx, by);
                g.setColour(Palette::accent);
                g.fillPath(mark);
            }
        }
        g.setColour(Palette::border);
        g.drawHorizontalLine((int) bottom.getBottom(), bottom.getX(), bottom.getRight());
        g.setColour(Palette::text);
        g.strokePath(level, juce::PathStrokeType(2.0f));
    }

private:
    static void label(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text) {
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.5f));
        g.drawText(text, r.withHeight(12.0f), juce::Justification::centredLeft);
    }

    ControlShape shape_;
    std::vector<PressStep> steps_ = simulatePresses(ControlShape{});
};

inline constexpr std::array<int, 7> kExampleDetents{1, 1, 1, -1, 1, 1, -1};

inline int encoderByteFor(EncoderFormat f, int direction) {
    switch (f) {
        case EncoderFormat::TwosComplement: return direction > 0 ? 0x01 : 0x7F;
        case EncoderFormat::Offset64:       return direction > 0 ? 0x41 : 0x3F;
        case EncoderFormat::SignBit:        return direction > 0 ? 0x01 : 0x41;
    }
    return 0;
}

class EncoderStairs : public juce::Component {
public:
    void setShape(const ControlShape& s) { shape_ = s; repaint(); }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(4.0f);
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, 4.0f);
        r = r.reduced(12.0f, 10.0f);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.5f));
        g.drawText(tr("mapping-shape.each-click-sends", "each click sends"), r.removeFromTop(12.0f),
                   juce::Justification::centredLeft);
        auto boxes = r.removeFromTop(34.0f);
        r.removeFromTop(8.0f);
        g.drawText(tr("mapping-shape.parameter-moves", "and the parameter moves"), r.removeFromTop(12.0f),
                   juce::Justification::centredLeft);
        auto stairs = r.reduced(0.0f, 4.0f);
        const float slot = boxes.getWidth() / (float) kExampleDetents.size();
        double v = 0.45;
        juce::Path level;
        level.startNewSubPath(stairs.getX(), yOf(stairs, v));
        for (size_t i = 0; i < kExampleDetents.size(); ++i) {
            const int dir = kExampleDetents[i];
            const float x = boxes.getX() + slot * (float) i;
            auto box = juce::Rectangle<float>(x + 3.0f, boxes.getY() + 4.0f, slot - 6.0f, boxes.getHeight() - 8.0f);
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(box, 3.0f);
            g.setColour(dir > 0 ? Palette::accent : Palette::textDim);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(juce::String(dir > 0 ? "+ " : "- ")
                           + juce::String::toHexString(encoderByteFor(shape_.encoder, dir)).paddedLeft('0', 2).toUpperCase(),
                       box, juce::Justification::centred);
            const double move = encoderAction(shape_, dir).value;
            v = std::clamp(v + move * kStairScale, 0.0, 1.0);
            level.lineTo(x + slot * 0.5f, level.getCurrentPosition().y);
            level.lineTo(x + slot * 0.5f, yOf(stairs, v));
        }
        level.lineTo(stairs.getRight(), level.getCurrentPosition().y);
        g.setColour(Palette::border);
        g.drawHorizontalLine((int) stairs.getBottom(), stairs.getX(), stairs.getRight());
        g.setColour(Palette::text);
        g.strokePath(level, juce::PathStrokeType(2.0f));
    }

private:
    static constexpr double kStairScale = 2.0;
    static float yOf(juce::Rectangle<float> r, double v) { return r.getBottom() - (float) v * r.getHeight(); }

    ControlShape shape_;
};

}
