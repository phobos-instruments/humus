// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/tracks/TimelineChips.h"

#include "hum/dsp/FadeLaw.h"

namespace hum::timelinechrome {

inline juce::String warpLabel(double sourceBpm, double projectBpm, int mode, bool full = true) {
    if (mode == 0 || sourceBpm <= 0.0) return {};
    juce::String l(juce::String::fromUTF8("\xe2\x97\x86 "));
    l << juce::String(sourceBpm, 1);
    if (full && std::abs(sourceBpm - projectBpm) > 0.05)
        l << juce::String::fromUTF8(" \xe2\x86\x92 ") << juce::String(projectBpm, 1);
    return l;
}

inline void paintWarpBadge(juce::Graphics& g, juce::Rectangle<int> clip,
                           double sourceBpm, double projectBpm, int mode,
                           juce::Colour accent) {
    if (mode == 0 || sourceBpm <= 0.0 || clip.getHeight() < 22) return;
    g.setFont(juce::FontOptions(9.0f));
    auto label = warpLabel(sourceBpm, projectBpm, mode, true);
    int w = (int) textWidth(g.getCurrentFont(), label) + 7;
    if (clip.getWidth() < w + 6) {
        label = warpLabel(sourceBpm, projectBpm, mode, false);
        w = (int) textWidth(g.getCurrentFont(), label) + 7;
        if (clip.getWidth() < w + 6) return;
    }
    const juce::Rectangle<int> r(clip.getRight() - w - 3, clip.getBottom() - 13, w, 11);
    g.setColour(Palette::panel.withAlpha(alpha::heavy));
    g.fillRoundedRectangle(r.toFloat(), 2.0f);
    g.setColour(accent);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 2.0f, 1.0f);
    g.setColour(Palette::text);
    g.drawText(label, r, juce::Justification::centred);
}

inline void paintFades(juce::Graphics& g, juce::Rectangle<int> clip,
                       int fadeInTicks, int fadeOutTicks, int lengthTicks,
                       juce::Colour accent, double inCurve = 0.0, double outCurve = 0.0) {
    if (lengthTicks <= 0 || clip.getWidth() < 6) return;
    const auto b = clip.toFloat();
    const float perTick = b.getWidth() / (float) lengthTicks;

    auto ramp = [&](float xa, float xb, bool rising, double curve) {
        juce::Path p;
        constexpr int steps = 24;
        for (int i = 0; i <= steps; ++i) {
            const float t = (float) i / (float) steps;
            const float gain = fadeGain(rising ? t : 1.0f - t, (float) curve);
            p.lineTo(xa + (xb - xa) * t, b.getBottom() - gain * b.getHeight());
            if (i == 0) { p.clear(); p.startNewSubPath(xa, b.getBottom() - gain * b.getHeight()); }
        }
        return p;
    };
    auto wedge = [&](juce::Path r, float cornerX) {
        g.setColour(accent.brighter(0.4f));
        g.strokePath(r, juce::PathStrokeType(1.2f));
        r.lineTo(cornerX, b.getY());
        r.closeSubPath();
        g.setColour(Palette::background.withAlpha(alpha::mid));
        g.fillPath(r);
    };
    if (fadeInTicks > 0) {
        const float w = juce::jmin(b.getWidth(), (float) fadeInTicks * perTick);
        wedge(ramp(b.getX(), b.getX() + w, true, inCurve), b.getX());
    }
    if (fadeOutTicks > 0) {
        const float w = juce::jmin(b.getWidth(), (float) fadeOutTicks * perTick);
        wedge(ramp(b.getRight() - w, b.getRight(), false, outCurve), b.getRight());
    }
}

enum class FadeGrip { None, Left, Right };

inline FadeGrip fadeGripAt(juce::Rectangle<int> clip, juce::Point<int> p, int grip,
                           int fadeInTicks = 0, int fadeOutTicks = 0, int lengthTicks = 0) {
    if (clip.getWidth() <= 3 * grip) return FadeGrip::None;
    if (p.y < clip.getY() || p.y >= clip.getY() + grip) return FadeGrip::None;
    if (p.x < clip.getX() || p.x >= clip.getRight()) return FadeGrip::None;
    const float per = lengthTicks > 0 ? (float) clip.getWidth() / (float) lengthTicks : 0.0f;
    const auto span = [&](int ticks) {
        return (int) juce::jmin((float) clip.getWidth(), (float) ticks * per);
    };
    const int lx = clip.getX() + span(fadeInTicks);
    const int rx = clip.getRight() - span(fadeOutTicks);
    const int dl = std::abs(p.x - lx), dr = std::abs(p.x - rx);
    if (dl <= grip && dl <= dr) return FadeGrip::Left;
    if (dr <= grip) return FadeGrip::Right;
    return FadeGrip::None;
}

inline FadeGrip fadeCurveGripAt(juce::Rectangle<int> clip, juce::Point<int> p, int grip,
                               int fadeInTicks, int fadeOutTicks, int lengthTicks,
                               double inCurve, double outCurve) {
    if (lengthTicks <= 0 || clip.getWidth() <= 3 * grip) return FadeGrip::None;
    if (!clip.contains(p)) return FadeGrip::None;
    const float per = (float) clip.getWidth() / (float) lengthTicks;
    const auto span = [&](int ticks) {
        return juce::jmin((float) clip.getWidth(), (float) ticks * per);
    };
    const auto onRamp = [&](float xa, float xb, bool rising, double curve) {
        if (xb - xa < 1.0f || p.x < xa + grip || p.x > xb - grip) return false;
        const float t = ((float) p.x - xa) / (xb - xa);
        const float y = (float) clip.getBottom()
                      - fadeGain(rising ? t : 1.0f - t, (float) curve) * (float) clip.getHeight();
        return std::abs((float) p.y - y) <= (float) grip;
    };
    if (fadeInTicks > 0
        && onRamp((float) clip.getX(), (float) clip.getX() + span(fadeInTicks), true, inCurve))
        return FadeGrip::Left;
    if (fadeOutTicks > 0
        && onRamp((float) clip.getRight() - span(fadeOutTicks), (float) clip.getRight(), false,
                  outCurve))
        return FadeGrip::Right;
    return FadeGrip::None;
}

inline void paintFadeCurveGrips(juce::Graphics& g, juce::Rectangle<int> clip,
                                int fadeInTicks, int fadeOutTicks, int lengthTicks,
                                double inCurve, double outCurve, juce::Colour accent) {
    if (lengthTicks <= 0 || clip.getWidth() < 12) return;
    const auto b = clip.toFloat();
    const float per = b.getWidth() / (float) lengthTicks;
    const auto dot = [&](float xa, float xb, double curve) {
        const float mid = (xa + xb) * 0.5f;
        const float y = b.getBottom() - fadeGain(0.5f, (float) curve) * b.getHeight();
        g.setColour(Palette::background);
        g.fillEllipse(mid - 3.5f, y - 3.5f, 7.0f, 7.0f);
        g.setColour(accent.brighter(0.6f));
        g.fillEllipse(mid - 2.5f, y - 2.5f, 5.0f, 5.0f);
    };
    if (fadeInTicks > 0) {
        const float w = juce::jmin(b.getWidth(), (float) fadeInTicks * per);
        dot(b.getX(), b.getX() + w, inCurve);
    }
    if (fadeOutTicks > 0) {
        const float w = juce::jmin(b.getWidth(), (float) fadeOutTicks * per);
        dot(b.getRight() - w, b.getRight(), outCurve);
    }
}

inline double fadeCurveFromDrag(double startCurve, int dyPixels, int clipHeight) {
    if (clipHeight <= 0) return startCurve;
    return juce::jlimit(-1.0, 1.0, startCurve - (double) dyPixels / (double) clipHeight * 2.0);
}

inline void paintFadeGrips(juce::Graphics& g, juce::Rectangle<int> clip, int grip,
                           int fadeInTicks, int fadeOutTicks, juce::Colour accent) {
    if (clip.getWidth() < 3 * grip || clip.getHeight() < 2 * grip) return;
    const auto b = clip.toFloat();
    g.setColour(accent.withAlpha(alpha::dim));
    const float y = b.getY() + 2.5f;
    if (fadeInTicks <= 0)
        g.drawLine(b.getX() + 2.0f, y + (float) grip - 4.0f, b.getX() + (float) grip - 2.0f, y, 1.2f);
    if (fadeOutTicks <= 0)
        g.drawLine(b.getRight() - (float) grip + 2.0f, y, b.getRight() - 2.0f, y + (float) grip - 4.0f, 1.2f);
}

inline void paintRepeatGrip(juce::Graphics& g, juce::Rectangle<int> clip, int grip,
                            juce::Colour accent, bool hot) {
    if (clip.getWidth() < 3 * grip || clip.getHeight() < 2 * grip) return;
    const auto b = clip.toFloat();
    g.setColour(accent.withAlpha(hot ? 0.95f : 0.5f));
    for (int i = 0; i < 2; ++i) {
        const float x = b.getRight() - 3.5f - (float) i * 3.5f;
        g.drawLine(x, b.getBottom() - 3.0f, x + 2.5f, b.getBottom() - (float) grip + 1.0f, 1.2f);
    }
}

}
