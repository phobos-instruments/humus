#include "hum/dsp/FadeLaw.h"
#include "AudioTrack/AudioTrack.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace hum {

void AudioTrack::prepare(double sampleRate, int) {
    if (sampleRate != sampleRate_) cache_.clear();
    sampleRate_ = sampleRate;
    meter_.prepare(sampleRate);
    clipsDirty_ = true;
    ensureClipsLoaded();
    if (hasPending_.load()) applyPending();
    reset();
}

void AudioTrack::reset() {
    for (auto& c : clips_) c.readPos = -1;
    lastBlockEndBeat_ = -1.0;
}

void AudioTrack::loadFrom(const OrganismState& state) {
    Organism::loadFrom(state);
    pattern_ = state.pattern;
    clipsDirty_ = true;
}

void AudioTrack::process(const float* const* in, int numIn,
                         float* const* out, int numOut,
                         int numSamples, const Transport& t) {
    if (hasPending_.load(std::memory_order_acquire)) applyPending();

    const bool playing = t.playing();
    const bool armed = params.get("Record", 0.0) >= 0.5;
    const int monitor = (int) params.get("Monitor", 1.0);
    const float gain = (float) params.get("Gain", 1.0);
    const bool mute = params.get("Mute", 0.0) >= 0.5;

    if (takeActive_.load(std::memory_order_relaxed) && playing && numIn > 0 && in[0]) {
        const double beat = t.beats();
        if (takeLen_.load(std::memory_order_relaxed) == 0) {
            takeStartBeat_.store(beat, std::memory_order_relaxed);
        } else if (beat < lastCaptureBeat_) {
            const int n = lapCount_.load(std::memory_order_relaxed);
            if (n < kMaxLaps) {
                lapBeats_[(size_t) n] = beat;
                lapSamples_[(size_t) n] = takeLen_.load(std::memory_order_relaxed);
                lapCount_.store(n + 1, std::memory_order_relaxed);
            }
        }
        lastCaptureBeat_ = beat;
        const float* chans[2] = { in[0], numIn > 1 && in[1] ? in[1] : in[0] };
        writer_.write(chans, numSamples);
        takeLen_.fetch_add(numSamples, std::memory_order_relaxed);
    }

    const bool passInput = monitor == 0 || (monitor == 1 && (armed || !playing));
    const bool playClips = playing && (monitor == 0 || !passInput);

    for (int c = 0; c < numOut; ++c) {
        if (passInput && c < numIn && in[c])
            std::memcpy(out[c], in[c], sizeof(float) * (size_t) numSamples);
        else
            std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    }
    if (playClips) renderClips(out, numOut, numSamples, t);
    else for (auto& c : clips_) c.readPos = -1;

    if (mute) {
        for (int c = 0; c < numOut; ++c)
            std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    } else if (gain != 1.0f) {
        for (int c = 0; c < numOut; ++c)
            for (int n = 0; n < numSamples; ++n) out[c][n] *= gain;
    }
    meter_.measure(out, numOut, numSamples);
}

static double warpRate(int mode, double sourceBpm, double projectBpm) {
    if (mode == (int) PatternChannel::Warp::Off) return 1.0;
    if (sourceBpm <= 0.0 || projectBpm <= 0.0) return 1.0;
    return projectBpm / sourceBpm;
}

void AudioTrack::renderClips(float* const* out, int numOut, int numSamples,
                             const Transport& t) {
    const double beat0 = t.beats();
    const double spb = t.samplesPerBeat();
    const double projectBpm = t.tempo();
    const bool jumped = std::abs(beat0 - lastBlockEndBeat_) > 1e-9;

    for (auto& cp : clips_) {
        if (!cp.pcm) continue;
        int startN = 0;
        if (cp.startBeat > beat0)
            startN = (int) std::ceil((cp.startBeat - beat0) * spb - 1e-6);
        int endN = numSamples;
        {
            const double e = (cp.startBeat + cp.lenBeats - beat0) * spb + 1e-6;
            if (e < (double) numSamples) endN = (int) e;
        }
        if (startN >= endN || startN >= numSamples) { cp.readPos = -1; continue; }

        const std::int64_t loopSpan =
            cp.loop ? std::max<std::int64_t>(1, (std::int64_t) std::llround(cp.periodBeats * spb))
                    : 0;
        cp.rate = warpRate(cp.warpMode, cp.sourceBpm, projectBpm);
        const double off = (double) cp.offset;
        if (cp.readPos < 0.0 || jumped || startN > 0) {
            double rb = (beat0 - cp.clipStartBeat) + startN / spb;
            if (rb * spb < -1e-6) { cp.readPos = -1.0; continue; }
            if (rb < 0.0) rb = 0.0;
            if (cp.loop) rb = std::fmod(rb, cp.periodBeats);
            cp.readPos = off + rb * spb * cp.rate;
        }

        const auto& pcm = *cp.pcm;
        const std::int64_t pcmLen = pcm.getNumSamples();
        const double loopEnd = off + (double) loopSpan * cp.rate;
        const float g = (float) cp.gain;
        const double fadeIn = cp.fadeInBeats, fadeOut = cp.fadeOutBeats;
        const bool anyFade = fadeIn > 0.0 || fadeOut > 0.0;
        const double regionLen = cp.periodBeats * spb * cp.rate;
        const bool beats = cp.warpMode == (int) PatternChannel::Warp::Beats;
        const double shiftRatio = cp.pitchRatio / (beats ? cp.rate : 1.0);
        const bool shifting = std::abs(shiftRatio - 1.0) > 1e-9;
        if (shifting) for (auto& s : cp.shift) s.setRatio(shiftRatio);
        const double lead = shifting ? cp.shiftLatency * cp.rate : 0.0;
        for (int n = startN; n < endN; ++n) {
            if (cp.loop && cp.readPos >= loopEnd) cp.readPos = off;
            const double src = (cp.reverse ? off + regionLen - 1.0 - (cp.readPos - off) : cp.readPos)
                               + (cp.reverse ? -lead : lead);
            if (cp.readPos >= 0.0 && src >= 0.0 && src < (double) pcmLen) {
                const std::int64_t i0 = (std::int64_t) src;
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
                for (int c = 0; c < numOut; ++c) {
                    const int sc = pcm.getNumChannels() > c ? c : pcm.getNumChannels() - 1;
                    const float a = pcm.getSample(sc, (int) i0);
                    const float b = pcm.getSample(sc, (int) i1);
                    const float v = a + (float) fr * (b - a);
                    out[c][n] += (shifting && c < 2 ? cp.shift[(size_t) c].process(v) : v) * gn;
                }
            }
            cp.readPos += cp.rate;
        }
    }
    lastBlockEndBeat_ = beat0 + numSamples / spb;
}

void AudioTrack::applyPending() {
    const juce::ScopedLock sl(loadLock_);
    clips_.swap(pendingClips_);
    hasPending_.store(false, std::memory_order_release);
}

bool AudioTrack::startTake(const std::string& wavPath, double sampleRate) {
    if (writer_.active()) return false;
    if (!writer_.start(wavPath, 2, sampleRate)) return false;
    takePath_ = wavPath;
    takeLen_.store(0, std::memory_order_relaxed);
    lapCount_.store(0, std::memory_order_relaxed);
    takeStartBeat_.store(-1.0, std::memory_order_relaxed);
    lastCaptureBeat_ = -1.0;
    takeActive_.store(true, std::memory_order_relaxed);
    return true;
}

void AudioTrack::stopTake() {
    takeActive_.store(false, std::memory_order_relaxed);
    writer_.stop();
}

int AudioTrack::takeLaps(double* beats, std::int64_t* samples, int maxLaps) const {
    const int n = std::min(maxLaps, lapCount_.load(std::memory_order_relaxed));
    for (int i = 0; i < n; ++i) {
        beats[i] = lapBeats_[(size_t) i];
        samples[i] = lapSamples_[(size_t) i];
    }
    return n;
}

}
