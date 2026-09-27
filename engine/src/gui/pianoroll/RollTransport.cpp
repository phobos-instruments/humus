// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/RollModel.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "hum/Pattern.h"

namespace hum::roll {

double clipTick(const std::vector<ClipSpan>& clips, int clip, double beats) {
    if (clip < 0 || clip >= (int) clips.size()) return -1.0;
    const auto& c = clips[(size_t) clip];
    const double rel = beats * Pattern::kTicksPerBeat - c.startTick;
    if (rel < 0.0) return -1.0;
    if (c.looped) return std::fmod(rel, (double) c.lengthTicks);
    return rel < (double) c.lengthTicks ? rel : -1.0;
}

Playhead playheadAt(const std::vector<ClipSpan>& clips, int clip, double beats, int durationTicks, bool playing) {
    if (const double onClip = clipTick(clips, clip, beats); onClip >= 0.0) return {onClip, false, playing};
    if (!clips.empty()) return {-1.0, false, playing};
    const double span = (double) std::max(1, durationTicks);
    return {std::fmod(std::max(0.0, beats) * Pattern::kTicksPerBeat, span), true, playing};
}

int durationOf(const std::vector<ClipSpan>& clips, int clip, int patternTicks) {
    if (clip >= 0 && clip < (int) clips.size()) return clips[(size_t) clip].lengthTicks;
    if (patternTicks > 0) return patternTicks;
    return 4 * 4 * Pattern::kTicksPerBeat;
}

LoopTap loopTapFor(bool armed, bool takeOpen, bool hasNotes) {
    if (armed) return takeOpen ? LoopTap::CloseTake : LoopTap::Disarm;
    return hasNotes ? LoopTap::Overdub : LoopTap::StartTake;
}

}
