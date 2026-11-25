// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
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

void AudioTrack::reset() { player_.reset(); }

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
    if (playClips) player_.render(out, numOut, numSamples, t.beats(), t.samplesPerBeat(), t.tempo());
    else player_.rest();

    if (mute) {
        for (int c = 0; c < numOut; ++c)
            std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    } else if (gain != 1.0f) {
        for (int c = 0; c < numOut; ++c)
            for (int n = 0; n < numSamples; ++n) out[c][n] *= gain;
    }
    meter_.measure(out, numOut, numSamples);
}

void AudioTrack::applyPending() {
    const juce::ScopedTryLock sl(loadLock_);
    if (!sl.isLocked()) return;
    player_.voices().swap(pendingClips_);
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
