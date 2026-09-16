// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "VideoTrack/VideoTrack.h"

#include <algorithm>
#include <cmath>

#include "hum/ClipStack.h"
#include "hum/dsp/FadeLaw.h"

#include "hum/dsp/DspMath.h"

namespace hum {

std::vector<VideoClipPlay> videoClipPlays(const Pattern& pattern, double sampleRate) {
    std::vector<VideoClipPlay> out;
    const double tpb = Pattern::kTicksPerBeat;
    for (int i = 0; i < (int) pattern.channels.size(); ++i) {
        const auto& ch = pattern.channels[(size_t) i];
        if ((ch.type != "video-clip" && ch.type != "compound-clip")
            || ch.startTick < 0 || ch.audioFile.empty()) continue;
        for (const auto& span : clipstack::soundingSpans(pattern, i)) {
            VideoClipPlay cp;
            cp.id = ch.id;
            cp.startBeat = span.startTick / tpb;
            cp.lenBeats = std::max(1, span.lengthTicks) / tpb;
            cp.clipStartBeat = ch.startTick / tpb;
            cp.periodBeats = std::max(1, ch.lengthTicks) / tpb;
            cp.loop = ch.loopClip;
            cp.offsetSeconds = sampleRate > 0.0 ? (double) ch.audioOffset / sampleRate : 0.0;
            cp.gain = ch.audioGain;
            cp.sourceBpm = ch.sourceBpm;
            cp.warpMode = ch.warpMode;
            cp.fadeInBeats = ch.fadeInTicks / tpb;
            cp.fadeOutBeats = ch.fadeOutTicks / tpb;
            cp.fadeInCurve = (float) ch.fadeInCurve;
            cp.fadeOutCurve = (float) ch.fadeOutCurve;
            cp.reverse = ch.audioReverse;
            cp.file = ch.audioFile;
            out.push_back(std::move(cp));
        }
    }
    return out;
}

namespace {

double playRate(const VideoClipPlay& cp, double tempo) {
    return cp.warpMode != 0 && cp.sourceBpm > 0.0 ? tempo / cp.sourceBpm : 1.0;
}

VideoTimelineSource::Cue cueFor(const VideoClipPlay& cp, double beat, double tempo, bool rolling) {
    VideoTimelineSource::Cue c;
    c.clip = cp.id;
    c.rolling = rolling;
    const double spb = kSecondsPerMinute / std::max(1.0, tempo);
    double rel = beat - cp.clipStartBeat;
    if (cp.loop && cp.periodBeats > 0.0) rel = std::fmod(rel, cp.periodBeats);
    rel = std::max(0.0, rel);
    const double rate = playRate(cp, tempo);
    const double t = rel * spb * rate;
    const double lenSeconds = cp.periodBeats * spb * rate;
    c.seconds = cp.offsetSeconds + (cp.reverse ? std::max(0.0, lenSeconds - t) : t);
    c.rate = cp.reverse ? -rate : rate;
    float level = (float) cp.gain;
    if (cp.fadeInBeats > 0.0)
        level *= fadeGain((float) (rel / cp.fadeInBeats), cp.fadeInCurve);
    if (cp.fadeOutBeats > 0.0) {
        const double left = cp.periodBeats - rel;
        level *= fadeGain((float) (left / cp.fadeOutBeats), cp.fadeOutCurve);
    }
    c.level = std::clamp(level, 0.0f, 1.0f);
    return c;
}

}

VideoTimelineSource::Cue videoCueAt(const std::vector<VideoClipPlay>& clips, double beat,
                                    double tempo, bool rolling) {
    const VideoClipPlay* best = nullptr;
    for (const auto& cp : clips)
        if (beat >= cp.startBeat && beat < cp.startBeat + cp.lenBeats
            && (best == nullptr || cp.startBeat >= best->startBeat))
            best = &cp;
    if (best == nullptr) return {};
    return cueFor(*best, beat, tempo, rolling);
}

VideoTimelineSource::Cue videoUpcomingAt(const std::vector<VideoClipPlay>& clips, double beat,
                                         double tempo, double windowSeconds) {
    const double windowBeats = windowSeconds * std::max(1.0, tempo) / kSecondsPerMinute;
    const VideoClipPlay* next = nullptr;
    for (const auto& cp : clips)
        if (cp.startBeat > beat && cp.startBeat - beat <= windowBeats
            && (next == nullptr || cp.startBeat < next->startBeat))
            next = &cp;
    if (next == nullptr) return {};
    auto c = cueFor(*next, next->startBeat, tempo, false);
    c.rate = 0.0;
    return c;
}

void VideoTrack::loadFrom(const OrganismState& state) {
    Organism::loadFrom(state);
    setPattern(state.pattern);
}

void VideoTrack::setPattern(const Pattern& pattern) {
    pattern_ = pattern;
    auto next = videoClipPlays(pattern_, sampleRate_);
    const juce::ScopedLock sl(loadLock_);
    table_ = next;
    pending_.swap(next);
    hasPending_.store(true, std::memory_order_release);
}

void VideoTrack::process(const float* const*, int, float* const*, int, int,
                         const Transport& transport) {
    if (hasPending_.exchange(false, std::memory_order_acq_rel)) {
        const juce::ScopedLock sl(loadLock_);
        clips_.swap(pending_);
    }
    const double beat = transport.beats();
    const double tempo = transport.tempo();
    const bool rolling = transport.playing();
    publish(cueClip_, cueSeconds_, cueRate_, cueLevel_, videoCueAt(clips_, beat, tempo, rolling));
    publish(nextClip_, nextSeconds_, nextRate_, nextLevel_,
            videoUpcomingAt(clips_, beat, tempo, kPrerollSeconds));
    rolling_.store(rolling, std::memory_order_relaxed);
}

void VideoTrack::publish(std::atomic<int>& clip, std::atomic<double>& seconds,
                         std::atomic<double>& rate, std::atomic<float>& level, const Cue& c) {
    seconds.store(c.seconds, std::memory_order_relaxed);
    rate.store(c.rate, std::memory_order_relaxed);
    level.store(c.level, std::memory_order_relaxed);
    clip.store(c.clip, std::memory_order_release);
}

VideoTimelineSource::Cue VideoTrack::cue() const {
    Cue c;
    c.clip = cueClip_.load(std::memory_order_acquire);
    c.seconds = cueSeconds_.load(std::memory_order_relaxed);
    c.rate = cueRate_.load(std::memory_order_relaxed);
    c.level = cueLevel_.load(std::memory_order_relaxed);
    c.rolling = rolling_.load(std::memory_order_relaxed);
    return c;
}

VideoTimelineSource::Cue VideoTrack::upcoming() const {
    Cue c;
    c.clip = nextClip_.load(std::memory_order_acquire);
    c.seconds = nextSeconds_.load(std::memory_order_relaxed);
    c.rate = 0.0;
    c.level = nextLevel_.load(std::memory_order_relaxed);
    return c;
}

VideoTimelineSource::Cue VideoTrack::cueAt(double beat, double tempo) const {
    const juce::ScopedLock sl(loadLock_);
    return videoCueAt(table_, beat, tempo, false);
}

VideoTimelineSource::Cue VideoTrack::upcomingAt(double beat, double tempo) const {
    const juce::ScopedLock sl(loadLock_);
    return videoUpcomingAt(table_, beat, tempo, kPrerollSeconds);
}

std::string VideoTrack::cueFile(int clip) const {
    const juce::ScopedLock sl(loadLock_);
    for (const auto& cp : table_)
        if (cp.id == clip) return cp.file;
    return {};
}

}
