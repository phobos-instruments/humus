// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/tracks/TracksLayout.h"
#include "io/MeterMap.h"

#include "hum/dsp/DspMath.h"

namespace hum::timelinechrome {

template <class Fn>
void forEachBar(const MeterMap& meters, double fromBeat, double toBeat, Fn&& fn) {
    double b = meters.barStartBefore(std::max(0.0, fromBeat));
    for (int guard = 0; guard < 100000 && b <= toBeat; ++guard) {
        const double next = meters.nextBarStart(b);
        fn(b, next, meters.at(b));
        if (next <= b) break;
        b = next;
    }
}

inline void paintTimeGrid(juce::Graphics& g, juce::Rectangle<int> area, int leftEdge,
                          double scrollBeats, double ppb, const MeterMap& meters,
                          double gridChoice) {
    if (ppb <= 0.0 || area.getHeight() <= 0) return;
    const auto cb = g.getClipBounds();
    const auto right = (float) std::min(area.getRight(), cb.getRight());
    const auto x0 = (float) std::max(leftEdge, cb.getX());
    auto xOf = [&](double beat) { return (float) (leftEdge + (beat - scrollBeats) * ppb); };
    auto stroke = [&](float x) {
        if (x >= x0 && x <= right)
            g.drawVerticalLine((int) x, (float) area.getY(), (float) area.getBottom());
    };
    const double lastBeat = scrollBeats + (right - (float) leftEdge) / ppb;
    forEachBar(meters, scrollBeats, lastBeat, [&](double start, double next, Meter m) {
        const double grid = gridChoice > 0.0 ? gridChoice : trackslayout::gridBeats(ppb, m);
        if (grid > 0.0 && grid < next - start && grid * ppb >= 4.0) {
            g.setColour(Palette::border.withAlpha(alpha::mist));
            for (double b = start + grid; b < next - 1.0e-9; b += grid) stroke(xOf(b));
        }
        g.setColour(Palette::border.withAlpha(alpha::scrim));
        stroke(xOf(start));
    });
}

inline void paintTimeGrid(juce::Graphics& g, juce::Rectangle<int> area, int leftEdge,
                          double scrollBeats, double ppb, int beatsPerBar,
                          double gridBeats) {
    if (beatsPerBar <= 0) return;
    paintTimeGrid(g, area, leftEdge, scrollBeats, ppb,
                  MeterMap({{0.0, Meter{beatsPerBar, 4}}}), gridBeats);
}

inline juce::String positionLabel(int bar, double beatInBar) {
    const double snapped = std::round(beatInBar * 100.0) / 100.0;
    juce::String beat(snapped, 2);
    while (beat.endsWith("0")) beat = beat.dropLastCharacters(1);
    if (beat.endsWith(".")) beat = beat.dropLastCharacters(1);
    return juce::String(bar) + ":" + beat;
}

inline bool parsePositionLabel(const juce::String& text, const MeterMap& meters, double& beat) {
    const auto t = text.trim();
    if (t.isEmpty()) return false;
    const int colon = t.indexOfChar(':');
    const int bar = (colon < 0 ? t : t.substring(0, colon)).getIntValue();
    const double inBar = colon < 0 ? 1.0 : t.substring(colon + 1).getDoubleValue();
    if (bar < 1 || inBar < 1.0) return false;
    const double start = meters.barStart(bar - 1);
    beat = start + (inBar - 1.0) * meters.at(start).quarterNotesPerBeat();
    return beat >= 0.0;
}

inline juce::String barLabel(int barNumber, double beat, double tempo, float barPixels) {
    juce::String label(barNumber);
    if (barPixels > 64.0f) {
        const double bpm = tempo > 0.0 ? tempo : 120.0;
        const int secs = (int) std::floor(beat * kSecondsPerMinute / bpm);
        label << "  " << juce::String(secs / 60) << ":"
              << juce::String(secs % 60).paddedLeft('0', 2);
    }
    return label;
}

inline int barLabelStride(float barPixels, float minSpacing = 46.0f) {
    if (barPixels <= 0.0f) return 1;
    int stride = 1;
    while (stride < 4096 && (float) stride * barPixels < minSpacing) stride *= 2;
    return stride;
}

inline void paintSongEnd(juce::Graphics& g, float x, float rulerTop, float bottom,
                         float stripW, float right) {
    if (x < stripW || x > right) return;
    const auto red = Palette::recordRed();
    g.setColour(red.withAlpha(alpha::mid));
    g.drawLine(x, rulerTop, x, bottom, 1.2f);
    juce::Path flag;
    flag.addTriangle(x, rulerTop + 2.0f, x, rulerTop + 12.0f, x - 8.0f, rulerTop + 7.0f);
    g.setColour(red);
    g.fillPath(flag);
}

inline void paintPlayhead(juce::Graphics& g, float x, float top, float bottom,
                          float stripW, float right, bool grabber) {
    if (x < stripW || x > right) return;
    g.setColour(Palette::accent.withAlpha(alpha::mist));
    g.fillRect(x - 2.5f, top, 5.0f, bottom - top);
    g.setColour(Palette::accent);
    g.fillRect(x - 0.5f, top, 1.0f, bottom - top);
    if (!grabber) return;
    juce::Path tri;
    tri.addTriangle(x - 4.5f, top, x + 4.5f, top, x, top + 7.0f);
    g.fillPath(tri);
}

}
