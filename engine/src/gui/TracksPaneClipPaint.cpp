#include "gui/TracksPane.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "gui/LookAndFeel.h"
#include "gui/WaveformCache.h"

namespace hum {

void TracksPane::paintClip(juce::Graphics& g) {
    const auto f = clipField();
    ClipEditor::ClipInfo ci;
    g.setColour(Palette::background);
    g.fillRect(0, headerH(), getWidth(), getHeight() - headerH());
    if (!clipInfo(ci)) return;
    const auto accent = timelinechrome::laneAccent(host_.model(), clipNode_);
    const auto box = clipBox();

    g.setColour(accent.withAlpha(0.05f));
    g.fillRect(f);
    g.setColour(accent.withAlpha(0.13f));
    g.fillRect(box.getIntersection(f));
    g.setColour(Palette::border.withAlpha(0.5f));
    g.drawHorizontalLine(f.getCentreY(), (float) f.getX(), (float) f.getRight());

    g.saveState();
    g.reduceClipRegion(f);
    paintClipWave(g, f, ci, accent);
    timelinechrome::paintFades(g, box, ci.fadeInTicks, ci.fadeOutTicks, ci.lengthTicks, accent,
                               ci.fadeInCurve, ci.fadeOutCurve);
    timelinechrome::paintFadeGrips(g, box, kFadeGrip * 2, ci.fadeInTicks, ci.fadeOutTicks, accent);
    timelinechrome::paintFadeCurveGrips(g, box, ci.fadeInTicks, ci.fadeOutTicks, ci.lengthTicks,
                                        ci.fadeInCurve, ci.fadeOutCurve, accent);
    g.setColour(accent);
    g.fillRect(box.getX() - 1, f.getY(), 2, f.getHeight());
    g.fillRect(box.getRight() - 1, f.getY(), 2, f.getHeight());
    g.setColour(Palette::text.withAlpha(0.55f));
    for (const auto s : clipTransients(ci)) {
        const float x = tickToX((int) sampleToTick(ci, s));
        g.fillRect(x - 0.5f, (float) f.getY(), 1.0f, 8.0f);
        g.fillRect(x - 0.5f, (float) f.getBottom() - 8.0f, 1.0f, 8.0f);
    }

    if (hasSel_ && selTo_ > selFrom_) {
        const float x0 = beatToX(selFrom_), x1 = beatToX(selTo_);
        g.setColour(Palette::accent.withAlpha(0.18f));
        g.fillRect(x0, (float) f.getY(), x1 - x0, (float) f.getHeight());
        g.setColour(Palette::accent.withAlpha(0.7f));
        g.drawVerticalLine((int) x0, (float) f.getY(), (float) f.getBottom());
        g.drawVerticalLine((int) x1, (float) f.getY(), (float) f.getBottom());
    }
    g.restoreState();

    paintClipRibbon(g, ci);
    paintClipCrumb(g);
    paintCutGuide(g);
    timelinechrome::paintPlayhead(g, beatToX(playBeat_), (float) rulerTop(), (float) f.getBottom(),
                                  (float) kStripW, (float) getWidth(), true);
}

void TracksPane::paintClipWave(juce::Graphics& g, juce::Rectangle<int> f,
                               const ClipEditor::ClipInfo& ci, juce::Colour accent) {
    const auto* peaks = WaveformCache::instance().get(ci.audioFile, [this] { repaint(); });
    if (!peaks || !peaks->ready || peaks->sourceSamples <= 0) return;
    const double spb = samplesPerBeat();
    const double fileRate = peaks->fileSampleRate > 0.0 ? peaks->fileSampleRate : host_.sampleRate();
    const double toFile = fileRate / host_.sampleRate();
    const double startBeat = ci.startTick / (double) Pattern::kTicksPerBeat;
    const double endBeat = (ci.startTick + ci.lengthTicks) / (double) Pattern::kTicksPerBeat;
    const double regionLen = ci.lengthTicks / (double) Pattern::kTicksPerBeat * spb;
    auto fileSampleAtX = [&](int x) {
        const double rel = (xToBeat((float) x) - startBeat) * spb;
        return ((double) ci.audioOffset + (ci.audioReverse ? regionLen - rel : rel)) * toFile;
    };
    auto xOfFileSample = [&](double s) {
        const double rel = s / toFile - (double) ci.audioOffset;
        return beatToX(startBeat + (ci.audioReverse ? regionLen - rel : rel) / spb);
    };
    const double perPx = std::abs(fileSampleAtX(f.getX() + 1) - fileSampleAtX(f.getX()));
    const float midY = (float) f.getCentreY();
    const float halfH = f.getHeight() * 0.5f - 3.0f;
    const float gain = (float) ci.audioGain;

    auto colourAt = [&](int x, bool body) {
        const double b = xToBeat((float) x);
        const bool inside = b >= startBeat && b < endBeat;
        const auto base = body ? accent.brighter(0.35f) : accent;
        return base.withAlpha(inside ? (body ? 0.8f : 0.45f) : (body ? 0.22f : 0.12f));
    };

    if (perPx >= 256.0 && peaks->binSamples > 0) {
        const bool haveRms = peaks->rms.size() == peaks->hi.size();
        for (int x = f.getX(); x < f.getRight(); ++x) {
            const double sa = fileSampleAtX(x), sb = fileSampleAtX(x + 1);
            const int b0 = (int) std::floor(std::min(sa, sb) / peaks->binSamples);
            const int b1 = std::max(b0 + 1, (int) std::floor(std::max(sa, sb) / peaks->binSamples));
            float hi = 0.0f, lo = 0.0f, rms = 0.0f;
            int n = 0;
            for (int k = std::max(0, b0); k < std::min(b1, (int) peaks->hi.size()); ++k) {
                hi = std::max(hi, peaks->hi[(size_t) k]);
                lo = std::min(lo, peaks->lo[(size_t) k]);
                if (haveRms) rms = std::max(rms, peaks->rms[(size_t) k]);
                ++n;
            }
            if (n == 0) continue;
            g.setColour(colourAt(x, false));
            g.drawVerticalLine(x, midY - hi * gain * halfH, midY - lo * gain * halfH + 1.0f);
            if (rms > 0.002f) {
                g.setColour(colourAt(x, true));
                g.drawVerticalLine(x, midY - rms * gain * halfH, midY + rms * gain * halfH);
            }
        }
        return;
    }

    const auto from = (std::int64_t) std::floor(std::min(fileSampleAtX(f.getX()), fileSampleAtX(f.getRight())));
    const auto to = (std::int64_t) std::ceil(std::max(fileSampleAtX(f.getX()), fileSampleAtX(f.getRight()))) + 1;
    const auto& mono = clipWindow_.read(ci.audioFile, from, to);
    if (mono.empty()) return;
    const auto base = clipWindow_.from();
    auto sampleAt = [&](std::int64_t s) -> float {
        const auto i = s - base;
        return i >= 0 && i < (std::int64_t) mono.size() ? mono[(size_t) i] * gain : 0.0f;
    };
    if (perPx >= 3.0) {
        for (int x = f.getX(); x < f.getRight(); ++x) {
            const double sa = fileSampleAtX(x), sb = fileSampleAtX(x + 1);
            const auto s0 = (std::int64_t) std::floor(std::min(sa, sb));
            const auto s1 = std::max(s0 + 1, (std::int64_t) std::floor(std::max(sa, sb)));
            float hi = -1.0f, lo = 1.0f;
            for (auto s = s0; s < s1; ++s) { const float v = sampleAt(s); hi = std::max(hi, v); lo = std::min(lo, v); }
            if (hi < lo) continue;
            g.setColour(colourAt(x, true));
            g.drawVerticalLine(x, midY - hi * halfH, midY - lo * halfH + 1.0f);
        }
        return;
    }
    juce::Path line;
    bool started = false;
    const double pxPerSample = 1.0 / std::max(1e-9, perPx);
    for (auto s = from; s <= to; ++s) {
        const float x = xOfFileSample((double) s);
        const float y = midY - sampleAt(s) * halfH;
        if (!started) { line.startNewSubPath(x, y); started = true; }
        else line.lineTo(x, y);
        if (pxPerSample >= 6.0) {
            g.setColour(colourAt((int) x, false));
            g.fillEllipse(x - 1.5f, y - 1.5f, 3.0f, 3.0f);
        }
    }
    g.setColour(accent.brighter(0.35f).withAlpha(0.8f));
    g.strokePath(line, juce::PathStrokeType(1.0f));
}

void TracksPane::paintClipCrumb(juce::Graphics& g) {
    ClipEditor::ClipInfo ci;
    paintBackCrumb(g);
    const juce::Rectangle<int> name(crumbBackBox().getRight() + 10, 1, 300, kTopH - 2);
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(timelinechrome::laneAccent(host_.model(), clipNode_));
    g.fillEllipse((float) name.getX(), (float) name.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setColour(Palette::text);
    juce::String label(clipNode_);
    if (clipInfo(ci)) label += juce::String::fromUTF8(" \xe2\x80\xba ") + (ci.name.empty() ? juce::File(ci.audioFile).getFileNameWithoutExtension() : juce::String(ci.name));
    g.drawText(label, name.withTrimmedLeft(10).withWidth(300), juce::Justification::centredLeft, true);
}

void TracksPane::paintClipRibbon(juce::Graphics& g, const ClipEditor::ClipInfo& ci) {
    const auto f = clipField();
    const juce::Rectangle<int> r(kStripW, f.getBottom(), getWidth() - kStripW,
                                 fieldBottom() - f.getBottom());
    g.setColour(Palette::panel);
    g.fillRect(r);
    g.setColour(Palette::border);
    g.drawHorizontalLine(r.getY(), (float) r.getX(), (float) r.getRight());
    const double sr = host_.sampleRate() > 0.0 ? host_.sampleRate() : 48000.0;
    const double lenBeats = ci.lengthTicks / (double) Pattern::kTicksPerBeat;
    juce::String s = juce::File(ci.audioFile).getFileName()
                     + "   start " + juce::String((double) ci.audioOffset / sr, 3) + " s"
                     + "   length " + juce::String(lenBeats, 2) + " beats"
                     + "   gain " + clipdetail::gainDb(ci.audioGain)
                     + (ci.audioReverse ? "   reversed" : "")
                     + (ci.audioPitch != 0.0 ? "   pitch " + juce::String(ci.audioPitch, 0) + " st" : "")
                     + (ci.warpMode == (int) PatternChannel::Warp::Beats ? "   beats warp" : "");
    int from = 0, to = 0;
    if (clipSelectionTicks(from, to))
        s += "   selection " + juce::String((to - from) / (double) Pattern::kTicksPerBeat, 3) + " beats";
    s += "   |   drag: select   edges: trim   cmd-edge: stretch   alt-drag: slip   S: split   Tab: next hit   Z: zoom   L: loop";
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(s, r.reduced(6, 0), juce::Justification::centredLeft, true);
}

}
