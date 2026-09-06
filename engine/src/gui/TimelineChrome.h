#pragma once
#include <cmath>
#include <cstddef>
#include <map>
#include <juce_gui_basics/juce_gui_basics.h>

#include "hum/dsp/FadeLaw.h"

#include "core/Categories.h"
#include "core/CordTrace.h"
#include "gui/LookAndFeel.h"
#include "gui/NoteEdit.h"
#include "gui/TracksLayout.h"

#include "hum/dsp/DspMath.h"

namespace hum::timelinechrome {

inline void paintTimeGrid(juce::Graphics& g, juce::Rectangle<int> area, int leftEdge,
                          double scrollBeats, double ppb, int beatsPerBar,
                          double gridBeats) {
    if (ppb <= 0.0 || beatsPerBar <= 0 || area.getHeight() <= 0) return;
    const auto cb = g.getClipBounds();
    const auto right = (float) std::min(area.getRight(), cb.getRight());
    const auto x0 = (float) std::max(leftEdge, cb.getX());
    auto xOf = [&](double beat) { return (float) (leftEdge + (beat - scrollBeats) * ppb); };
    auto stroke = [&](float x) {
        if (x >= x0 && x <= right)
            g.drawVerticalLine((int) x, (float) area.getY(), (float) area.getBottom());
    };

    if (gridBeats > 0.0 && gridBeats < (double) beatsPerBar
        && gridBeats * ppb >= 4.0) {
        g.setColour(Palette::border.withAlpha(0.12f));
        for (int i = std::max(0, (int) (scrollBeats / gridBeats));; ++i) {
            const double b = i * gridBeats;
            const float x = xOf(b);
            if (x > right) break;
            if (std::abs(b / beatsPerBar - std::round(b / beatsPerBar)) < 1.0e-9) continue;
            stroke(x);
        }
    }
    g.setColour(Palette::border.withAlpha(0.3f));
    for (int bar = std::max(0, (int) (scrollBeats / beatsPerBar));; ++bar) {
        const float x = xOf(bar * (double) beatsPerBar);
        if (x > right) break;
        stroke(x);
    }
}

inline void paintToolIcon(juce::Graphics& g, juce::Rectangle<float> r,
                          noteedit::Tool t) {
    if (t == noteedit::Tool::Pointer) {
        juce::Path a;
        a.startNewSubPath(r.getX() + 1.0f, r.getY());
        a.lineTo(r.getX() + 1.0f, r.getBottom() - 1.0f);
        a.lineTo(r.getX() + 4.0f, r.getBottom() - 4.0f);
        a.lineTo(r.getRight() - 2.0f, r.getBottom() + 1.0f);
        g.strokePath(a, juce::PathStrokeType(1.4f));
    } else if (t == noteedit::Tool::Draw) {
        juce::Path pen;
        pen.startNewSubPath(r.getX() + 1.0f, r.getBottom());
        pen.lineTo(r.getX() + 3.0f, r.getBottom() - 3.0f);
        pen.lineTo(r.getRight(), r.getY() + 1.0f);
        pen.lineTo(r.getRight() - 3.0f, r.getY() - 1.0f);
        pen.lineTo(r.getX() + 1.0f, r.getBottom());
        g.strokePath(pen, juce::PathStrokeType(1.3f));
    } else if (t == noteedit::Tool::Line) {
        g.drawLine(r.getX() + 1.0f, r.getBottom() - 1.0f, r.getRight() - 1.0f, r.getY() + 1.0f, 1.4f);
        g.fillEllipse(r.getX() - 1.0f, r.getBottom() - 3.0f, 4.0f, 4.0f);
        g.fillEllipse(r.getRight() - 3.0f, r.getY() - 1.0f, 4.0f, 4.0f);
    } else if (t == noteedit::Tool::Scissors) {
        g.drawLine(r.getX(), r.getY(), r.getRight(), r.getBottom(), 1.4f);
        g.drawLine(r.getRight(), r.getY(), r.getX(), r.getBottom(), 1.4f);
        g.fillEllipse(r.getX() - 1.0f, r.getBottom() - 2.0f, 4.0f, 4.0f);
        g.fillEllipse(r.getRight() - 3.0f, r.getBottom() - 2.0f, 4.0f, 4.0f);
    } else {
        juce::Path er;
        er.addRoundedRectangle(r.getX(), r.getCentreY() - 3.0f, r.getWidth(), 7.0f, 2.0f);
        g.strokePath(er, juce::PathStrokeType(1.3f),
                     juce::AffineTransform::rotation(-0.5f, r.getCentreX(), r.getCentreY()));
    }
}

inline const juce::MouseCursor& toolCursor(noteedit::Tool t) {
    static std::map<int, juce::MouseCursor> cache;
    auto it = cache.find((int) t);
    if (it != cache.end()) return it->second;
    if (t == noteedit::Tool::Pointer)
        return cache.emplace((int) t, juce::MouseCursor::NormalCursor).first->second;
    constexpr int kSize = 26;
    juce::Image img(juce::Image::ARGB, kSize, kSize, true);
    juce::Graphics g(img);
    const juce::Rectangle<float> r(6.0f, 6.0f, 14.0f, 14.0f);
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx != 0 || dy != 0) paintToolIcon(g, r.translated((float) dx, (float) dy), t);
    g.setColour(juce::Colours::black.withAlpha(0.9f));
    paintToolIcon(g, r, t);
    const bool tip = t == noteedit::Tool::Draw || t == noteedit::Tool::Line;
    return cache.emplace((int) t, juce::MouseCursor(img, tip ? 6 : kSize / 2, tip ? 20 : kSize / 2))
        .first->second;
}

inline float textWidth(const juce::Font& f, const juce::String& text) {
    return juce::GlyphArrangement::getStringWidth(f, text);
}

inline constexpr float kChipFont = 11.0f;
inline constexpr float kCrumbFont = 11.5f;

inline void paintSnapChip(juce::Graphics& g, juce::Rectangle<int> r,
                          double gridBeats, bool chosen, const char* labelOverride = nullptr) {
    if (r.getWidth() < 24 || r.getHeight() < 8) return;
    const auto label = labelOverride ? std::string(labelOverride) : trackslayout::gridLabel(gridBeats);
    g.setColour(Palette::panelLight);
    g.fillRoundedRectangle(r.toFloat(), 2.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 2.0f, 1.0f);
    g.setColour(chosen ? Palette::accent : Palette::textDim);
    g.setFont(juce::FontOptions(kChipFont));
    g.drawText(label, r, juce::Justification::centred);
    if (chosen) g.fillEllipse((float) r.getRight() - 5.0f, (float) r.getY() + 2.0f, 3.0f, 3.0f);
}

inline juce::Colour laneAccent(const PatchDocumentModel& m, const std::string& node) {
    const auto dest = cords::destinationOf(m, node);
    const auto* dcm = dest.empty() ? nullptr : m.byName(dest);
    if (dcm == nullptr) return Palette::accent;
    return Palette::familyAccent(familyOf(dcm->displayClass));
}

inline void paintDestChip(juce::Graphics& g, juce::Rectangle<int> r,
                          const PatchDocumentModel& m, const std::string& node) {
    const auto dest = cords::destinationOf(m, node);
    if (dest.empty() || r.getWidth() < 26) return;
    const auto* dcm = m.byName(dest);
    const auto label = juce::String::fromUTF8("\xe2\x86\x92 ")
                       + juce::String(dcm != nullptr && !pseudoOwnerLabel(dcm->displayClass).empty()
                                          ? pseudoOwnerLabel(dcm->displayClass)
                                          : dest);
    g.setFont(juce::FontOptions(9.5f));
    const int wanted = (int) textWidth(g.getCurrentFont(), label) + 8;
    r = r.withWidth(juce::jmin(r.getWidth(), juce::jmax(26, wanted)));
    g.setColour(Palette::border);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 2.0f, 1.0f);
    g.setColour(Palette::textDim);
    g.drawText(label, r.reduced(3, 0), juce::Justification::centredLeft, true);
}

inline juce::String midiDestLabel(const PatchDocumentModel& m, const std::string& node) {
    std::string dest;
    for (const auto& c : m.midiConnections)
        if (c.src == node && c.srcOutlet == 0) { dest = c.dst; break; }
    return juce::String::fromUTF8("\xe2\x86\x92 ")
           + (dest.empty() ? juce::String("(nothing)") : juce::String(dest))
           + juce::String::fromUTF8("  \xe2\x96\xbe");
}

inline juce::Rectangle<int> midiDestChipRect(juce::Rectangle<int> r,
                                             const PatchDocumentModel& m,
                                             const std::string& node) {
    const juce::Font f{juce::FontOptions(12.0f)};
    const int wanted = (int) textWidth(f, midiDestLabel(m, node)) + 8;
    return r.withWidth(juce::jmin(r.getWidth(), juce::jmax(26, wanted)));
}

inline void paintMidiDestChip(juce::Graphics& g, juce::Rectangle<int> r,
                              const PatchDocumentModel& m, const std::string& node) {
    if (r.getWidth() < 26) return;
    std::string dest;
    for (const auto& c : m.midiConnections)
        if (c.src == node && c.srcOutlet == 0) { dest = c.dst; break; }
    const auto label = midiDestLabel(m, node);
    g.setFont(juce::FontOptions(12.0f));
    r = midiDestChipRect(r, m, node);
    g.setColour(Palette::panelLight);
    g.fillRoundedRectangle(r.toFloat(), 2.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 2.0f, 1.0f);
    g.setColour(dest.empty() ? Palette::textDim : Palette::accent);
    g.drawText(label, r.reduced(3, 0), juce::Justification::centredLeft, true);
}

inline void paintHeldBadge(juce::Graphics& g, juce::Rectangle<int> r, bool on = true) {
    if (r.getWidth() < 9 || r.getHeight() < 9) return;
    const auto amber = Palette::warnAmber();
    g.setColour(on ? amber : Palette::panelLight);
    g.fillRect(r);
    g.setColour(on ? amber.darker(0.9f) : Palette::textDim);
    g.setFont(juce::FontOptions(9.5f));
    g.drawText("H", r, juce::Justification::centred);
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
    g.setColour(red.withAlpha(0.55f));
    g.drawLine(x, rulerTop, x, bottom, 1.2f);
    juce::Path flag;
    flag.addTriangle(x, rulerTop + 2.0f, x, rulerTop + 12.0f, x - 8.0f, rulerTop + 7.0f);
    g.setColour(red);
    g.fillPath(flag);
}

inline void paintPlayhead(juce::Graphics& g, float x, float top, float bottom,
                          float stripW, float right, bool grabber) {
    if (x < stripW || x > right) return;
    g.setColour(Palette::accent.withAlpha(0.13f));
    g.fillRect(x - 2.5f, top, 5.0f, bottom - top);
    g.setColour(Palette::accent);
    g.fillRect(x - 0.5f, top, 1.0f, bottom - top);
    if (!grabber) return;
    juce::Path tri;
    tri.addTriangle(x - 4.5f, top, x + 4.5f, top, x, top + 7.0f);
    g.fillPath(tri);
}

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
    g.setColour(Palette::panel.withAlpha(0.85f));
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
        g.setColour(Palette::background.withAlpha(0.55f));
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
    g.setColour(accent.withAlpha(0.5f));
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

struct NotePlot {
    float perTick = 0.0f;
    float top = 0.0f;
    float h = 0.0f;
    float rowH = 0.0f;
    int lo = 0, span = 1;
    bool single = false;
    bool dense = false;
    bool usable = false;

    float yFor(int pitch) const {
        if (single || span <= 0 || rowH >= h) return top + (h - rowH) * 0.5f;
        const float t = (float) (pitch - lo) / (float) span;
        return top + h - rowH - t * (h - rowH);
    }
};

inline NotePlot notePlot(juce::Rectangle<int> brick, int fullW, int lengthTicks,
                         int lo, int hi, int ticksPerBeat) {
    NotePlot np;
    if (fullW <= 0 || lengthTicks <= 0 || ticksPerBeat <= 0) return np;
    np.perTick = (float) fullW / (float) lengthTicks;
    np.top = (float) brick.getY() + 3.0f;
    np.h = (float) brick.getHeight() - 6.0f;
    if (np.h <= 2.0f) return np;
    np.lo = juce::jmin(lo, hi);
    np.single = hi == lo;
    np.span = juce::jmax(1, hi - lo);
    np.rowH = juce::jmax(1.4f, np.h / (float) (np.span + 1));
    np.dense = np.perTick * (float) ticksPerBeat < 8.0f;
    np.usable = true;
    return np;
}

inline bool isBlackKey(int pitch) {
    static const bool black[12] = {false, true, false, true, false, false,
                                   true, false, true, false, true, false};
    return black[(std::size_t) (((pitch % 12) + 12) % 12)];
}

struct RollPlot {
    float top = 0.0f, h = 0.0f;
    float rowH = 10.0f;
    int topPitch = 83;
    bool usable = false;

    float hFor(int) const { return rowH; }
    float yFor(int pitch) const { return top + (float) (topPitch - pitch) * rowH; }
    int pitchAt(float y) const {
        return topPitch - (int) std::floor((y - top) / juce::jmax(0.001f, rowH));
    }
    int rows() const { return (int) std::ceil(h / juce::jmax(0.001f, rowH)) + 1; }
};

inline RollPlot rollPlot(float top, float h, int loPitch, int hiPitch,
                         int scrollSemis = 0, float wantedRowH = 10.0f) {
    RollPlot rp;
    rp.top = top;
    rp.h = h;
    if (h < 12.0f) return rp;
    int lo = juce::jlimit(0, kMidiMax, juce::jmin(loPitch, hiPitch) - 2);
    int hi = juce::jlimit(0, kMidiMax, juce::jmax(loPitch, hiPitch) + 2);
    if (hi - lo < 11) hi = juce::jmin(kMidiMax, lo + 11);
    const int span = juce::jmax(1, hi - lo + 1);
    rp.rowH = juce::jlimit(3.0f, 24.0f, juce::jmin(wantedRowH, h / (float) span));
    const int visible = (int) std::floor(h / rp.rowH);
    rp.topPitch = juce::jlimit(0, kMidiMax, hi + std::max(0, (visible - span) / 2) + scrollSemis);
    rp.usable = true;
    return rp;
}

}
