// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/tracks/SongView.h"

#include <algorithm>
#include <cmath>
#include <functional>

#include "gui/style/LookAndFeel.h"
#include "gui/video/VideoThumbCache.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {

void paintThumbTiles(juce::Graphics& g, juce::Rectangle<int> inner, int clipLeft,
                     const VideoThumbCache::Strip& thumbs,
                     const std::function<double(int)>& secondsAt) {
    const int tileH = inner.getHeight();
    const int tileW = std::max(8, (int) std::lround((double) tileH * thumbs.width
                                                    / std::max(1, thumbs.height)));
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(inner);
    int x = clipLeft;
    if (inner.getX() > clipLeft) x = clipLeft + ((inner.getX() - clipLeft) / tileW) * tileW;
    for (; x < inner.getRight(); x += tileW) {
        const auto* img = VideoThumbCache::frameAt(thumbs, secondsAt(x + tileW / 2));
        if (img == nullptr) continue;
        g.drawImage(*img, juce::Rectangle<int>(x, inner.getY(), tileW, tileH).toFloat(),
                    juce::RectanglePlacement::fillDestination);
    }
}

}

bool SongView::paintTapeTiles(juce::Graphics& g, juce::Rectangle<int> inner,
                                const ClipEditor::ClipInfo& ci) {
    const auto* thumbs = VideoThumbCache::instance().get(ci.audioFile, [this] { repaint(); });
    if (thumbs == nullptr || !thumbs->ready || thumbs->frames.empty() || thumbs->width <= 0
        || inner.getHeight() < 10)
        return false;
    paintThumbTiles(g, inner, (int) std::floor(tickToX(ci.startTick)), *thumbs,
                    [this, &ci](int x) { return clipSecondsAtX(ci, x); });
    return true;
}

bool SongView::paintReelTiles(juce::Graphics& g, juce::Rectangle<int> inner,
                                const ClipEditor::ClipInfo& ci, int depth) {
    const double bpm = host().tempo() > 0.0 ? host().tempo() : 120.0;
    const double spt = host().sampleRate() * (kSecondsPerMinute / bpm) / Pattern::kTicksPerBeat;
    const int into = spt > 0.0 ? (int) std::llround((double) ci.audioOffset / spt) : 0;
    bool any = false;
    for (const auto& child : host().clips().list(ci.audioFile)) {
        if (child.audioFile.empty()) continue;
        auto shifted = child;
        shifted.startTick = ci.startTick + child.startTick - into;
        const int from = (int) std::floor(tickToX(shifted.startTick));
        const int to = (int) std::ceil(tickToX(shifted.startTick + shifted.lengthTicks));
        auto slice = inner.getIntersection({from, inner.getY(), std::max(1, to - from),
                                            inner.getHeight()});
        if (slice.isEmpty()) continue;
        if (child.isCompound) {
            if (depth < kMaxReelPaintDepth) any |= paintReelTiles(g, slice, shifted, depth + 1);
            continue;
        }
        const auto* thumbs = VideoThumbCache::instance().get(child.audioFile,
                                                             [this] { repaint(); });
        if (thumbs == nullptr || !thumbs->ready || thumbs->frames.empty() || thumbs->width <= 0)
            continue;
        paintThumbTiles(g, slice, from, *thumbs,
                        [this, shifted](int x) { return clipSecondsAtX(shifted, x); });
        any = true;
    }
    return any;
}

double SongView::clipSecondsAtX(const ClipEditor::ClipInfo& ci, int x) const {
    const double bpm = host().tempo() > 0.0 ? host().tempo() : 120.0;
    const double spb = kSecondsPerMinute / bpm;
    const double rate = ci.warpMode != 0 && ci.sourceBpm > 0.0 ? bpm / ci.sourceBpm : 1.0;
    const double startBeat = ci.startTick / (double) Pattern::kTicksPerBeat;
    const double len = ci.lengthTicks / (double) Pattern::kTicksPerBeat * spb * rate;
    double rel = (xToBeat((float) x) - startBeat) * spb * rate;
    if (ci.looped && len > 0.0) rel = std::fmod(std::max(0.0, rel), len);
    rel = std::clamp(rel, 0.0, len);
    const double in = (double) ci.audioOffset / host().sampleRate();
    return in + (ci.audioReverse ? len - rel : rel);
}

void SongView::paintFilmstrip(juce::Graphics& g, juce::Rectangle<int> b,
                                const ClipEditor::ClipInfo& ci, juce::Colour ink) {
    auto strip = b.reduced(2, 3);
    strip.removeFromTop(12);
    if (strip.getHeight() < 8) return;
    g.setColour(Palette::background.withAlpha(alpha::mid));
    g.fillRoundedRectangle(strip.toFloat(), 2.0f);
    const int hole = 3;
    const auto inner = strip.reduced(0, hole + 3);
    const bool tiled = ci.isCompound ? paintReelTiles(g, inner, ci)
                                     : paintTapeTiles(g, inner, ci);
    g.setColour(ink.withAlpha(alpha::muted));
    for (int x = strip.getX() + 3; x < strip.getRight() - 3; x += 9) {
        g.fillRect(x, strip.getY() + 2, hole, hole);
        g.fillRect(x, strip.getBottom() - 2 - hole, hole, hole);
    }
    if (tiled || ci.isCompound || strip.getHeight() <= 20 || strip.getWidth() <= 40) return;
    g.setColour(ink.withAlpha(alpha::strong));
    g.setFont(juce::FontOptions(9.0f));
    const auto file = juce::File(juce::String(juce::CharPointer_UTF8(ci.audioFile.c_str())));
    g.drawText(file.getFileNameWithoutExtension(), strip.reduced(6, hole + 3),
               juce::Justification::centredLeft, true);
}

}
