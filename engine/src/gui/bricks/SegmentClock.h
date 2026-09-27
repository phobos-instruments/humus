// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/LookAndFeel.h"

namespace hum {

namespace segment {

inline constexpr int kTop = 1 << 0;
inline constexpr int kTopRight = 1 << 1;
inline constexpr int kBottomRight = 1 << 2;
inline constexpr int kBottom = 1 << 3;
inline constexpr int kBottomLeft = 1 << 4;
inline constexpr int kTopLeft = 1 << 5;
inline constexpr int kMiddle = 1 << 6;
inline constexpr int kAll = kTop | kTopRight | kBottomRight | kBottom | kBottomLeft | kTopLeft | kMiddle;

inline constexpr float kDigitAspect = 0.56f;
inline constexpr float kPunctAspect = 0.26f;
inline constexpr float kGapAspect = 0.10f;
inline constexpr float kStrokeAspect = 0.13f;

inline int maskFor(char c) {
    switch (c) {
        case '0': return kAll & ~kMiddle;
        case '1': return kTopRight | kBottomRight;
        case '2': return kTop | kTopRight | kMiddle | kBottomLeft | kBottom;
        case '3': return kTop | kTopRight | kMiddle | kBottomRight | kBottom;
        case '4': return kTopLeft | kTopRight | kMiddle | kBottomRight;
        case '5': return kTop | kTopLeft | kMiddle | kBottomRight | kBottom;
        case '6': return kAll & ~kTopRight;
        case '7': return kTop | kTopRight | kBottomRight;
        case '8': return kAll;
        case '9': return kAll & ~kBottomLeft;
        case '-': return kMiddle;
        default: return 0;
    }
}

inline bool isPunctuation(char c) { return c == ':' || c == '.'; }

inline float widthOf(const std::string& text, float height) {
    float w = 0.0f;
    for (size_t i = 0; i < text.size(); ++i) {
        w += (isPunctuation(text[i]) ? kPunctAspect : kDigitAspect) * height;
        if (i + 1 < text.size()) w += kGapAspect * height;
    }
    return w;
}

inline void drawDigit(juce::Graphics& g, juce::Rectangle<float> r, int mask,
                      juce::Colour lit, juce::Colour ghost) {
    const float t = r.getHeight() * kStrokeAspect;
    const float arm = r.getHeight() * 0.5f - t * 1.5f;
    const float corner = t * 0.35f;
    const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
    const struct { int bit; juce::Rectangle<float> box; } bars[] = {
        {kTop, {x + t, y, w - 2.0f * t, t}},
        {kMiddle, {x + t, y + h * 0.5f - t * 0.5f, w - 2.0f * t, t}},
        {kBottom, {x + t, y + h - t, w - 2.0f * t, t}},
        {kTopLeft, {x, y + t, t, arm}},
        {kTopRight, {x + w - t, y + t, t, arm}},
        {kBottomLeft, {x, y + h * 0.5f + t * 0.5f, t, arm}},
        {kBottomRight, {x + w - t, y + h * 0.5f + t * 0.5f, t, arm}},
    };
    for (const auto& bar : bars) {
        g.setColour((mask & bar.bit) != 0 ? lit : ghost);
        g.fillRoundedRectangle(bar.box, corner);
    }
}

inline void drawPunctuation(juce::Graphics& g, juce::Rectangle<float> r, char c, juce::Colour lit) {
    const float t = r.getHeight() * kStrokeAspect;
    g.setColour(lit);
    const float cx = r.getCentreX() - t * 0.5f;
    if (c == ':') {
        g.fillRoundedRectangle({cx, r.getY() + r.getHeight() * 0.30f - t * 0.5f, t, t}, t * 0.35f);
        g.fillRoundedRectangle({cx, r.getY() + r.getHeight() * 0.70f - t * 0.5f, t, t}, t * 0.35f);
        return;
    }
    g.fillRoundedRectangle({cx, r.getBottom() - t, t, t}, t * 0.35f);
}

inline void drawText(juce::Graphics& g, juce::Rectangle<float> area, const std::string& text,
                     juce::Colour lit, juce::Colour ghost) {
    const float height = area.getHeight();
    float x = area.getRight() - widthOf(text, height);
    for (const char c : text) {
        const float w = (isPunctuation(c) ? kPunctAspect : kDigitAspect) * height;
        const juce::Rectangle<float> cell{x, area.getY(), w, height};
        if (isPunctuation(c)) drawPunctuation(g, cell, c, lit);
        else drawDigit(g, cell, maskFor(c), lit, ghost);
        x += w + kGapAspect * height;
    }
}

}

inline std::string segmentClockText(double seconds, bool withThousandths) {
    if (!(seconds > 0.0)) seconds = 0.0;
    const int total = (int) seconds;
    const int mins = total / 60;
    const int secs = total % 60;
    std::string out = std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
    if (!withThousandths) return out;
    const int thousandths = (int) std::floor((seconds - (double) total) * 1000.0 + 0.5);
    const auto capped = thousandths > 999 ? 999 : thousandths;
    std::string tail = std::to_string(capped);
    tail.insert(0, 3 - tail.size(), '0');
    return out + "." + tail;
}

class SegmentClock : public juce::Component {
public:
    void setTime(double position, double length) {
        if (position == position_ && length == length_) return;
        position_ = position;
        length_ = length;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(Palette::background.darker(0.4f));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

        auto inner = r.reduced(9.0f, 4.0f);
        const bool twoRows = inner.getHeight() >= 26.0f;
        auto totalRow = inner;
        if (twoRows) {
            totalRow = inner.removeFromBottom(juce::jmin(11.0f, inner.getHeight() * 0.34f));
            inner.removeFromBottom(2.0f);
        }

        const auto lit = Palette::accent;
        const auto widest = segmentClockText(juce::jmax(position_, length_), true);
        drawFitted(g, inner, segmentClockText(position_, true), widest, lit,
                   lit.withAlpha(alpha::mist));
        if (!twoRows) return;
        const auto total = segmentClockText(length_, false);
        drawFitted(g, totalRow, total, total, Palette::textDim,
                   Palette::textDim.withAlpha(alpha::mist));
    }

private:
    static void drawFitted(juce::Graphics& g, juce::Rectangle<float> area, const std::string& text,
                           const std::string& sizedFor, juce::Colour lit, juce::Colour ghost) {
        const float unit = segment::widthOf(sizedFor, 1.0f);
        if (unit <= 0.0f) return;
        const float height = juce::jmin(area.getHeight(), area.getWidth() / unit);
        segment::drawText(g, area.withSizeKeepingCentre(area.getWidth(), height), text, lit, ghost);
    }

    double position_ = -1.0, length_ = -1.0;
};

}
