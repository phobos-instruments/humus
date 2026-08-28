#pragma once
#include <algorithm>
#include <cstdint>

#include "core/ClipOps.h"

namespace hum::clipops {

inline bool slipClip(Pattern& p, int clip, std::int64_t deltaSamples) {
    auto* ch = clipChannel(p, clip);
    if (!ch || !isAudioClip(*ch)) return false;
    ch->audioOffset = std::max<std::int64_t>(0, ch->audioOffset + deltaSamples);
    return true;
}

inline bool setClipGain(Pattern& p, int clip, double gain) {
    auto* ch = clipChannel(p, clip);
    if (!ch || !isAudioClip(*ch)) return false;
    ch->audioGain = std::max(0.0, gain);
    return true;
}

inline bool trimClipTo(Pattern& p, int clip, int fromAbs, int toAbs, double samplesPerTick) {
    const int start = clipStart(p, clip), end = start + clipLength(p, clip);
    fromAbs = std::max(fromAbs, start);
    toAbs = std::min(toAbs, end);
    if (toAbs - fromAbs <= 0) return false;
    if (fromAbs > start) resizeClip(p, clip, end - fromAbs, true, samplesPerTick);
    if (toAbs < end) resizeClip(p, clip, toAbs - fromAbs, false, samplesPerTick);
    return true;
}

inline bool removeClipRange(Pattern& p, int clip, int fromAbs, int toAbs, bool ripple,
                            double samplesPerTick) {
    const int start = clipStart(p, clip), end = start + clipLength(p, clip);
    fromAbs = std::max(fromAbs, start);
    toAbs = std::min(toAbs, end);
    if (toAbs - fromAbs <= 0) return false;
    int victim = clip;
    if (fromAbs > start) victim = splitClip(p, clip, fromAbs, samplesPerTick);
    if (toAbs < end) splitClip(p, victim, toAbs, samplesPerTick);
    removeClip(p, victim);
    if (ripple) {
        const int gap = toAbs - fromAbs;
        for (int i = 0; i < clipCount(p); ++i)
            if (auto* ch = clipChannel(p, i); ch && ch->startTick >= toAbs)
                ch->startTick -= gap;
    }
    return true;
}

inline PatternChannel clipRangeCopy(const Pattern& p, int clip, int fromAbs, int toAbs,
                                    double samplesPerTick) {
    PatternChannel out;
    const auto* ch = clipChannel(p, clip);
    if (!ch) return out;
    Pattern scratch = p;
    const int i = clipIndexOfId(scratch, clipId(p, clip));
    if (!trimClipTo(scratch, i, fromAbs, toAbs, samplesPerTick)) return out;
    out = *clipChannel(scratch, i);
    out.fadeInTicks = out.fadeOutTicks = 0;
    out.loopClip = false;
    return out;
}

inline bool stretchClip(Pattern& p, int clip, int newLengthTicks, double projectBpm) {
    auto* ch = clipChannel(p, clip);
    if (!ch || !isAudioClip(*ch) || newLengthTicks <= 0 || projectBpm <= 0.0) return false;
    const int oldLen = clipLength(p, clip);
    const double bpm = ch->sourceBpm > 0.0 ? ch->sourceBpm : projectBpm;
    ch->sourceBpm = bpm * (double) oldLen / (double) newLengthTicks;
    ch->warpMode = (int) PatternChannel::Warp::Beats;
    ch->lengthTicks = newLengthTicks;
    return true;
}

}
