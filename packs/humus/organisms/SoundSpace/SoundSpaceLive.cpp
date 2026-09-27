// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "SoundSpace/SoundSpace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "hum/dsp/DspMath.h"

namespace hum {

static_assert(SoundMapSource::kLiveFile == SoundSpace::kFiles);

void SoundSpace::writeLive(const float* left, const float* right, int numSamples, float gain) {
    const auto ring = (std::int64_t) liveLeft_.size();
    const std::int64_t written = liveWritten_.load(std::memory_order_relaxed);
    auto at = (size_t) (written % ring);
    for (int n = 0; n < numSamples; ++n) {
        liveLeft_[at] = gain * left[n];
        liveRight_[at] = gain * right[n];
        if (++at == (size_t) ring) at = 0;
    }
    liveWritten_.store(written + numSamples, std::memory_order_release);
}

void SoundSpace::feedLiveBack(const float* left, const float* right, int numSamples, float amount) {
    const auto ring = (std::int64_t) liveLeft_.size();
    const std::int64_t written = liveWritten_.load(std::memory_order_relaxed);
    if (ring == 0 || written < numSamples) return;
    auto at = (size_t) ((written - numSamples) % ring);
    for (int n = 0; n < numSamples; ++n) {
        liveLeft_[at] += std::tanh(amount * left[n]);
        liveRight_[at] += std::tanh(amount * right[n]);
        if (++at == (size_t) ring) at = 0;
    }
}

double SoundSpace::samplesToGrid(const Transport& transport, int offset) const {
    static constexpr double kGridBeats[] = {0.0, 0.125, 0.25, 0.5, 1.0};
    const int choice = (int) std::lround(params.get("Sync", 0.0));
    if (choice <= 0 || !transport.playing()) return 0.0;
    const double perBeat = transport.samplesPerBeat();
    const double grid = choice >= 5 ? transport.beatsPerBar() : kGridBeats[choice];
    const double beat = transport.beats() + (double) offset / perBeat;
    const double next = std::ceil(beat / grid - 1.0e-9) * grid;
    return (next - beat) * perBeat;
}

void SoundSpace::forgetLive() {
    for (auto& g : liveGrains_) g.start.store(-1);
    liveWas_ = false;
}

void SoundSpace::rebuildDisplay(std::int64_t written) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const std::int64_t fresh = written - (std::int64_t) (kLiveFreshSeconds * sr);
    displayPoints_ = corpusPoints_;
    for (const auto& g : liveGrains_) {
        const std::int64_t start = g.start.load();
        if (start < 0) continue;
        displayPoints_.push_back({g.x.load(), g.y.load(),
                                  start >= fresh ? SoundMapSource::kFreshLiveFile : SoundMapSource::kLiveFile});
    }
    ++generation_;
}

void SoundSpace::liveTick() {
    const auto ring = (std::int64_t) liveLeft_.size();
    if (!liveWanted() || ring == 0) {
        if (liveWas_) { forgetLive(); rebuildDisplay(liveWritten_.load()); }
        return;
    }
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const std::int64_t written = liveWritten_.load(std::memory_order_acquire);
    const auto window = (std::int64_t) (kLiveWindowSeconds * sr);
    if (!liveAnalyser_) liveAnalyser_.reset(makeAnalyser());
    if (!liveWas_ || written - liveAnalysed_ > window) {
        liveAnalysed_ = liveWas_ ? written - window : written;
        forgetPrevious(*liveAnalyser_);
        liveWas_ = true;
    }

    bool changed = false;
    std::array<float, kWindow> mono{};
    for (; liveAnalysed_ + kWindow <= written; liveAnalysed_ += kWindow / 2) {
        float energy = 0.0f;
        for (int i = 0; i < kWindow; ++i) {
            const auto at = (size_t) ((liveAnalysed_ + i) % ring);
            mono[(size_t) i] = 0.5f * (liveLeft_[at] + liveRight_[at]);
            energy += mono[(size_t) i] * mono[(size_t) i];
        }
        if (energy <= 0.0f) continue;
        const Feature feature = measureLive(*liveAnalyser_, mono.data(), sr);
        if (feature[0] < kLiveFloor) continue;
        auto& grain = liveGrains_[(size_t) liveSlot_];
        float x = 0.5f, y = 0.5f;
        projection_.place(feature, x, y);
        liveFeatures_[(size_t) liveSlot_] = feature;
        grain.start.store(-1);
        grain.x.store(x);
        grain.y.store(y);
        grain.start.store(liveAnalysed_);
        liveSlot_ = (liveSlot_ + 1) % kLiveGrains;
        changed = true;
    }

    const double freshSpan = kLiveFreshSeconds * sr;
    for (auto& g : liveGrains_) {
        const std::int64_t start = g.start.load();
        if (start < 0) continue;
        if (start < written - window) { g.start.store(-1); changed = true; }
        else if ((double) (written - start) < freshSpan * 2.0) changed = true;
    }
    if (changed) rebuildDisplay(written);
}

}
