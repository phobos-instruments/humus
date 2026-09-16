// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/ClipEditorView.h"
#include "gui/host/TracksHost.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "gui/tracks/WaveformCache.h"
#include "io/PatchDocument.h"

namespace hum {

int ClipEditorView::transientCount() {
    ClipEditor::ClipInfo ci;
    return clipInfo(ci) ? (int) clipTransients(ci).size() : 0;
}

long long ClipEditorView::sampleToTick(const ClipEditor::ClipInfo& ci, long long s) const {
    const double spt = samplesPerBeat() / Pattern::kTicksPerBeat;
    return ci.startTick + (long long) std::llround((double) (s - ci.audioOffset) / spt);
}

std::vector<long long> ClipEditorView::clipTransients(const ClipEditor::ClipInfo& ci) {
    std::vector<long long> out;
    const auto* peaks = WaveformCache::instance().get(ci.audioFile, [this] { repaint(); });
    if (!peaks || !peaks->ready) return out;
    const double toFile = (peaks->fileSampleRate > 0.0 ? peaks->fileSampleRate : host().sampleRate())
                          / host().sampleRate();
    const auto clipEnd = ci.audioOffset
                         + (long long) std::llround(ci.lengthTicks * samplesPerBeat() / Pattern::kTicksPerBeat);
    const float thresh = (float) (1.0 - sense_);
    for (const auto& o : peaks->onsets) {
        if (o.pos == 0 || o.strength < thresh) continue;
        const auto& win = window_.read(ci.audioFile, o.pos, o.pos + 2 * kOnsetHop);
        const long long filePos = win.empty() ? o.pos
                                              : window_.from() + refineOnset(win.data(), 0, (int) win.size());
        const auto s = (long long) std::llround((double) filePos / toFile);
        if (s > ci.audioOffset && s < clipEnd) out.push_back(s);
    }
    return out;
}

bool ClipEditorView::tabToTransient(int dir) {
    SelectionWatch watch(*this);
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return false;
    const auto ts = clipTransients(ci);
    const double spt = samplesPerBeat() / Pattern::kTicksPerBeat;
    const auto cur = ci.audioOffset
                     + (long long) std::llround((host().positionBeats() * Pattern::kTicksPerBeat - ci.startTick) * spt);
    const long long slack = (long long) (spt * 0.5);
    long long target = -1;
    if (dir > 0) { for (auto s : ts) if (s > cur + slack) { target = s; break; } }
    else { for (auto s : ts) if (s < cur - slack) target = s; }
    if (target < 0) {
        const auto edge = dir > 0 ? ci.startTick + ci.lengthTicks : ci.startTick;
        if (std::llabs((long long) std::llround(host().positionBeats() * Pattern::kTicksPerBeat) - edge) < 1) return false;
        host().setPositionBeats(edge / (double) Pattern::kTicksPerBeat);
        repaint();
        return true;
    }
    host().setPositionBeats(sampleToTick(ci, target) / (double) Pattern::kTicksPerBeat);
    repaint();
    return true;
}

void ClipEditorView::splitAtTransients() {
    SelectionWatch watch(*this);
    ClipEditor::ClipInfo ci;
    if (!clipInfo(ci)) return;
    int from = ci.startTick, to = ci.startTick + ci.lengthTicks;
    clipSelectionTicks(from, to);
    std::vector<int> ticks;
    for (auto s : clipTransients(ci)) {
        const auto t = sampleToTick(ci, s);
        if (t > from && t < to && (ticks.empty() || t != ticks.back())) ticks.push_back((int) t);
    }
    if (ticks.empty()) return;
    host().beginTransaction();
    host().pushUndo();
    for (auto it = ticks.rbegin(); it != ticks.rend(); ++it)
        if (const int c = clipOrdinal(); c >= 0) host().clips().split(node_, c, *it);
    host().endTransaction();
    view_.sel.active = false;
    repaint();
}

}
