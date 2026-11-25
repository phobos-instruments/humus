// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "hum/ClipStack.h"
#include "hum/Pattern.h"
#include "hum/Warp.h"
#include "hum/dsp/FadeLaw.h"
#include "hum/dsp/SpliceShifter.h"

namespace hum {

struct ClipPcm {
    std::vector<std::vector<float>> channels;
    std::int64_t length() const { return channels.empty() ? 0 : (std::int64_t) channels.front().size(); }
};

inline ClipPcm resampledPcm(const ClipPcm& src, double fromRate, double toRate) {
    const double ratio = fromRate / toRate;
    const auto srcLen = src.length();
    const auto outLen = std::max<std::int64_t>(1, (std::int64_t) std::floor((double) srcLen / ratio));
    ClipPcm out;
    for (const auto& s : src.channels) {
        auto& d = out.channels.emplace_back((std::size_t) outLen);
        for (std::int64_t n = 0; n < outLen; ++n) {
            const double pos = (double) n * ratio;
            const auto i = (std::int64_t) pos;
            const double f = pos - (double) i;
            const float a = s[(std::size_t) std::min(i, srcLen - 1)];
            const float b = s[(std::size_t) std::min(i + 1, srcLen - 1)];
            d[(std::size_t) n] = (float) (a + (b - a) * f);
        }
    }
    return out;
}

struct ClipVoice {
    double startBeat = 0.0, lenBeats = 0.0;
    bool loop = false;
    std::int64_t offset = 0;
    double clipStartBeat = 0.0, periodBeats = 0.0;
    double gain = 1.0;
    std::shared_ptr<const ClipPcm> pcm;
    double readPos = -1.0;
    double rate = 1.0;
    double sourceBpm = 0.0;
    int warpMode = 0;
    double fadeInBeats = 0.0, fadeOutBeats = 0.0;
    float fadeInCurve = 0.0f, fadeOutCurve = 0.0f;
    bool reverse = false;
    double pitchRatio = 1.0;
    SpliceShifter shift;
    int shiftLatency = 0;
};

inline constexpr double kClipShiftLatencySeconds = SpliceShifter::kCenterSeconds;

inline void appendClipVoices(std::vector<ClipVoice>& out, const Pattern& pattern, int index,
                             const std::shared_ptr<const ClipPcm>& pcm, double sampleRate) {
    const auto& ch = pattern.channels[(std::size_t) index];
    const double tpb = Pattern::kTicksPerBeat;
    for (const auto& span : clipstack::soundingSpans(pattern, index)) {
        ClipVoice v;
        v.startBeat = span.startTick / tpb;
        v.lenBeats = std::max(1, span.lengthTicks) / tpb;
        v.clipStartBeat = ch.startTick / tpb;
        v.periodBeats = std::max(1, ch.lengthTicks) / tpb;
        v.loop = ch.loopClip;
        v.offset = ch.audioOffset;
        v.gain = ch.audioGain;
        v.sourceBpm = ch.sourceBpm;
        v.warpMode = ch.warpMode;
        v.fadeInBeats = ch.fadeInTicks / tpb;
        v.fadeOutBeats = ch.fadeOutTicks / tpb;
        v.fadeInCurve = (float) ch.fadeInCurve;
        v.fadeOutCurve = (float) ch.fadeOutCurve;
        v.reverse = ch.audioReverse;
        v.pitchRatio = std::pow(2.0, ch.audioPitch / 12.0);
        v.shift.prepare(sampleRate);
        v.shiftLatency = (int) (sampleRate * kClipShiftLatencySeconds);
        v.pcm = pcm;
        out.push_back(std::move(v));
    }
}

class ClipPlayer {
public:
    std::vector<ClipVoice>& voices() { return voices_; }
    const std::vector<ClipVoice>& voices() const { return voices_; }

    void reset() {
        rest();
        lastBlockEndBeat_ = -1.0;
    }
    void rest() {
        for (auto& v : voices_) v.readPos = -1.0;
    }

    void render(float* const* out, int numOut, int numSamples, double beat0, double spb, double tempo) {
        const bool jumped = std::abs(beat0 - lastBlockEndBeat_) > 1e-9;
        for (auto& v : voices_) renderVoice(v, out, numOut, numSamples, beat0, spb, tempo, jumped);
        lastBlockEndBeat_ = beat0 + numSamples / spb;
    }

private:
    static void renderVoice(ClipVoice& cp, float* const* out, int numOut, int numSamples,
                            double beat0, double spb, double tempo, bool jumped) {
        if (!cp.pcm) return;
        int startN = 0;
        if (cp.startBeat > beat0)
            startN = (int) std::ceil((cp.startBeat - beat0) * spb - 1e-6);
        int endN = numSamples;
        {
            const double e = (cp.startBeat + cp.lenBeats - beat0) * spb + 1e-6;
            if (e < (double) numSamples) endN = (int) e;
        }
        if (startN >= endN || startN >= numSamples) { cp.readPos = -1; return; }

        const std::int64_t loopSpan =
            cp.loop ? std::max<std::int64_t>(1, (std::int64_t) std::llround(cp.periodBeats * spb)) : 0;
        cp.rate = warpRate(cp.warpMode, cp.sourceBpm, tempo);
        const double off = (double) cp.offset;
        if (cp.readPos < 0.0 || jumped || startN > 0) {
            double rb = (beat0 - cp.clipStartBeat) + startN / spb;
            if (rb * spb < -1e-6) { cp.readPos = -1.0; return; }
            if (rb < 0.0) rb = 0.0;
            if (cp.loop) rb = std::fmod(rb, cp.periodBeats);
            cp.readPos = off + rb * spb * cp.rate;
        }

        const auto& pcm = *cp.pcm;
        const std::int64_t pcmLen = pcm.length();
        const int pcmChannels = (int) pcm.channels.size();
        const double loopEnd = off + (double) loopSpan * cp.rate;
        const float g = (float) cp.gain;
        const double fadeIn = cp.fadeInBeats, fadeOut = cp.fadeOutBeats;
        const bool anyFade = fadeIn > 0.0 || fadeOut > 0.0;
        const double regionLen = cp.periodBeats * spb * cp.rate;
        const bool beats = cp.warpMode == (int) PatternChannel::Warp::Beats;
        const double shiftRatio = cp.pitchRatio / (beats ? cp.rate : 1.0);
        const bool shifting = std::abs(shiftRatio - 1.0) > 1e-9;
        if (shifting) cp.shift.setRatio(shiftRatio);
        const int shifted = std::min(numOut, SpliceShifter::kChannels);
        const double lead = shifting ? cp.shiftLatency * cp.rate : 0.0;
        for (int n = startN; n < endN; ++n) {
            if (cp.loop && cp.readPos >= loopEnd) cp.readPos = off;
            const double src = (cp.reverse ? off + regionLen - 1.0 - (cp.readPos - off) : cp.readPos)
                               + (cp.reverse ? -lead : lead);
            if (cp.readPos >= 0.0 && src >= 0.0 && src < (double) pcmLen) {
                const auto i0 = (std::int64_t) src;
                const double fr = src - (double) i0;
                const std::int64_t i1 = i0 + 1 < pcmLen ? i0 + 1 : i0;
                float gn = g;
                if (anyFade) {
                    const double inClip = cp.loop
                        ? std::fmod((cp.readPos - off) / (spb * cp.rate), cp.periodBeats)
                        : (beat0 + n / spb) - cp.clipStartBeat;
                    if (fadeIn > 0.0 && inClip < fadeIn)
                        gn *= fadeGain((float) std::max(0.0, inClip / fadeIn), cp.fadeInCurve);
                    if (fadeOut > 0.0 && inClip > cp.periodBeats - fadeOut)
                        gn *= fadeGain((float) std::max(0.0, (cp.periodBeats - inClip) / fadeOut),
                                       cp.fadeOutCurve);
                }
                float frame[SpliceShifter::kChannels] = {};
                for (int c = 0; c < numOut; ++c) {
                    const auto& chan = pcm.channels[(std::size_t) (pcmChannels > c ? c : pcmChannels - 1)];
                    const float a = chan[(std::size_t) i0];
                    const float b = chan[(std::size_t) i1];
                    const float v = a + (float) fr * (b - a);
                    if (shifting && c < shifted) frame[c] = v;
                    else out[c][n] += v * gn;
                }
                if (shifting) {
                    cp.shift.process(frame, shifted);
                    for (int c = 0; c < shifted; ++c) out[c][n] += frame[c] * gn;
                }
            }
            cp.readPos += cp.rate;
        }
    }

    std::vector<ClipVoice> voices_;
    double lastBlockEndBeat_ = -1.0;
};

}
