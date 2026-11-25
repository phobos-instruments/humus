// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/ClipEditorView.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "gui/host/EngineHostSamples.h"
#include "gui/host/TracksHost.h"
#include "gui/tracks/WaveformCache.h"

namespace hum {

bool ClipEditorView::waveGeometry(const ClipEditor::ClipInfo& ci, juce::Rectangle<int> f, WaveGeometry& wg) const {
    const auto* peaks = WaveformCache::instance().get(ci.audioFile, nullptr);
    if (peaks == nullptr || !peaks->ready) return false;
    const double hostRate = host().sampleRate();
    const double fileRate = peaks->fileSampleRate > 0.0 ? peaks->fileSampleRate : hostRate;
    wg.toFile = fileRate / hostRate;
    wg.spb = sourceSamplesPerBeat(ci);
    wg.startBeat = ci.startTick / (double) Pattern::kTicksPerBeat;
    wg.regionLen = ci.lengthTicks / (double) Pattern::kTicksPerBeat * wg.spb;
    wg.offset = ci.audioOffset;
    wg.reverse = ci.audioReverse;
    wg.midY = (float) f.getCentreY();
    wg.halfH = f.getHeight() * 0.5f - 3.0f;
    wg.gain = (float) ci.audioGain;
    return true;
}

double ClipEditorView::fileSampleAtX(const WaveGeometry& wg, float x) const {
    const double rel = (xToBeat(x) - wg.startBeat) * wg.spb;
    return ((double) wg.offset + (wg.reverse ? wg.regionLen - rel : rel)) * wg.toFile;
}

float ClipEditorView::xOfFileSample(const WaveGeometry& wg, double s) const {
    const double rel = s / wg.toFile - (double) wg.offset;
    return beatToX(wg.startBeat + (wg.reverse ? wg.regionLen - rel : rel) / wg.spb);
}

bool ClipEditorView::pencilReady(const ClipEditor::ClipInfo& ci) const {
    WaveGeometry wg;
    const auto f = clipField();
    if (ci.audioFile.empty() || !waveGeometry(ci, f, wg)) return false;
    const double samplesPerPixel = std::abs(fileSampleAtX(wg, (float) f.getX() + 1.0f) - fileSampleAtX(wg, (float) f.getX()));
    return samplesPerPixel * kPencilPixelsPerSample <= 1.0;
}

bool ClipEditorView::beginPencil(const ClipEditor::ClipInfo& ci, juce::Point<int> p) {
    if (!pencilReady(ci)) return false;
    strokePoints_.clear();
    drag_ = Drag::Pencil;
    dragPencil(p);
    return true;
}

void ClipEditorView::dragPencil(juce::Point<int> p) {
    ClipEditor::ClipInfo ci;
    WaveGeometry wg;
    if (!clipInfo(ci) || !waveGeometry(ci, clipField(), wg)) return;
    const auto sample = (std::int64_t) std::llround(fileSampleAtX(wg, (float) p.x));
    const float value = (wg.midY - (float) p.y) / wg.halfH / std::max(wg.gain, 1.0e-6f);
    strokePoints_.push_back({sample, std::clamp(value, -1.0f, 1.0f)});
    stroke_ = samplefix::pencilSpan(strokePoints_);
    repaint();
}

void ClipEditorView::endPencil() {
    drag_ = Drag::None;
    if (!stroke_.values.empty() && host().samples().drawSamples(node_, clipOrdinal(), stroke_.start, stroke_.values))
        ctx_.patchChanged();
    strokePoints_.clear();
    stroke_ = {};
    repaint();
}

bool ClipEditorView::selectionSamples(const ClipEditor::ClipInfo& ci, std::int64_t& start, int& length) const {
    WaveGeometry wg;
    if (!view_.sel.active || !waveGeometry(ci, clipField(), wg)) return false;
    const double a = fileSampleAtX(wg, beatToX(view_.sel.from));
    const double b = fileSampleAtX(wg, beatToX(view_.sel.to));
    start = std::max<std::int64_t>(0, (std::int64_t) std::floor(std::min(a, b)));
    const auto end = (std::int64_t) std::ceil(std::max(a, b));
    length = (int) std::max<std::int64_t>(0, end - start);
    return length > 0;
}

bool ClipEditorView::canRepairSelection() {
    ClipEditor::ClipInfo ci;
    WaveGeometry wg;
    std::int64_t start = 0;
    int length = 0;
    if (!clipInfo(ci) || ci.audioFile.empty() || !waveGeometry(ci, clipField(), wg)
        || !selectionSamples(ci, start, length))
        return false;
    const double fileRate = host().sampleRate() * wg.toFile;
    return length <= SampleEditor::kLongestRepairSeconds * fileRate;
}

void ClipEditorView::repairSelection() {
    ClipEditor::ClipInfo ci;
    std::int64_t start = 0;
    int length = 0;
    if (!canRepairSelection() || !clipInfo(ci) || !selectionSamples(ci, start, length)) return;
    if (host().samples().repairSamples(node_, clipOrdinal(), start, length)) ctx_.patchChanged();
    repaint();
}

void ClipEditorView::restoreOriginalAudio() {
    if (host().samples().restoreOriginal(node_, clipOrdinal())) ctx_.patchChanged();
    repaint();
}

}
