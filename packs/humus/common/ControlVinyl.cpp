// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/ControlVinyl.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "hum/dsp/DspMath.h"

extern "C" {
#include "timecoder.h"
}

namespace hum {

namespace {

constexpr int kChunk = 256;

constexpr double kToneFloorHz = 200.0;
constexpr double kSameSpeed = 0.08;
constexpr double kSteadyEnough = 0.03;
constexpr double kHoldCeilingSeconds = 8.0;
constexpr double kChallengerMinSeconds = 2.0;
constexpr double kFirstAcquireSeconds = 0.5;
constexpr double kSilenceIsARecordChange = 1.0;
constexpr double kHoldAfterSilence = 2.0;

const char* const kKnownFormats[] = {
    "serato_2a", "serato_2b", "serato_cd", "traktor_a", "traktor_b",
    "mixvibes_v2", "mixvibes_7inch", "pioneer_a", "pioneer_b",
};

signed short toPcm(float v) {
    const float clamped = std::clamp(v, -1.0f, 1.0f);
    return (signed short) std::lround(clamped * 32767.0f);
}

}

struct ControlVinyl::Impl {
    struct Candidate {
        std::string name;
        timecoder tc{};
        bool live = false;
    };

    std::vector<Candidate> candidates;
    std::vector<signed short> interleaved;
    double sampleRate = kDefaultSampleRate;
    int chosen = -1;
    int provisional = -1;
    std::int64_t pushed = 0;

    ~Impl() { clear(); }

    void clear() {
        for (auto& c : candidates)
            if (c.live) { timecoder_clear(&c.tc); c.live = false; }
        candidates.clear();
        chosen = -1;
        provisional = -1;
        pushed = 0;
    }

    double measuredHz() {
        if (candidates.empty()) return 0.0;
        const int at = chosen >= 0 ? chosen : provisional >= 0 ? provisional : 0;
        auto& tc = candidates[(std::size_t) at].tc;
        return timecoder_get_pitch(&tc) * timecoder_get_resolution(&tc);
    }

    void start(double sr, const std::string& format) {
        clear();
        sampleRate = sr > 0.0 ? sr : kDefaultSampleRate;
        for (const char* name : kKnownFormats) {
            if (!format.empty() && format != name) continue;
            auto* def = timecoder_find_definition(name);
            if (def == nullptr) continue;
            candidates.push_back({name});
            auto& c = candidates.back();
            timecoder_init(&c.tc, def, 1.0, (unsigned int) sampleRate, false);
            c.live = true;
        }
        if (candidates.size() == 1) chosen = 0;
    }
};

ControlVinyl::ControlVinyl() : impl_(std::make_unique<Impl>()) {}
ControlVinyl::~ControlVinyl() = default;

std::vector<std::string> ControlVinyl::formats() {
    std::vector<std::string> out;
    for (const char* name : kKnownFormats)
        if (timecoder_find_definition(name) != nullptr) out.emplace_back(name);
    return out;
}

void ControlVinyl::prepare(double sampleRate, const std::string& format) {
    impl_->start(sampleRate, format);
    settled_ = impl_->chosen >= 0 ? impl_->candidates[(size_t) impl_->chosen].name : std::string();
}

void ControlVinyl::reset() {
    const auto wanted = impl_->candidates.size() == 1 ? impl_->candidates[0].name : std::string();
    impl_->start(impl_->sampleRate, wanted);
    settled_ = impl_->chosen >= 0 ? impl_->candidates[(size_t) impl_->chosen].name : std::string();
}

void ControlVinyl::push(const float* left, const float* right, int numSamples) {
    if (impl_->candidates.empty() || numSamples <= 0) return;
    for (int at = 0; at < numSamples; at += kChunk) {
        const int n = std::min(kChunk, numSamples - at);
        impl_->interleaved.resize((std::size_t) n * 2);
        for (int i = 0; i < n; ++i) {
            impl_->interleaved[(std::size_t) i * 2] = toPcm(left[at + i]);
            impl_->interleaved[(std::size_t) i * 2 + 1] = toPcm(right[at + i]);
        }
        for (std::size_t c = 0; c < impl_->candidates.size(); ++c) {
            if (impl_->chosen >= 0 && (int) c != impl_->chosen) continue;
            timecoder_submit(&impl_->candidates[c].tc, impl_->interleaved.data(), (size_t) n);
            if (impl_->chosen < 0 && timecoder_get_position(&impl_->candidates[c].tc, nullptr) != -1) {
                impl_->chosen = (int) c;
                settled_ = impl_->candidates[c].name;
            }
        }
        impl_->pushed += n;
        learnRestTone(impl_->measuredHz(), (double) n / impl_->sampleRate);
        if (impl_->chosen < 0 && impl_->provisional < 0) latchProvisional();
    }
}

void ControlVinyl::learnRestTone(double hz, double seconds) {
    const double f = std::abs(hz);
    if (f < kToneFloorHz) {
        rest_.quietSeconds += seconds;
        if (rest_.quietSeconds > kSilenceIsARecordChange) {
            rest_.heldSeconds = std::min(rest_.heldSeconds, kHoldAfterSilence);
            rest_.challengerHz = 0.0;
            rest_.challengerHeldSeconds = 0.0;
        }
        return;
    }
    rest_.quietSeconds = 0.0;
    if (rest_.hz > 0.0 && std::abs(f / rest_.hz - 1.0) < kSameSpeed) {
        rest_.heldSeconds = std::min(rest_.heldSeconds + seconds, kHoldCeilingSeconds);
        rest_.challengerHeldSeconds = 0.0;
        return;
    }
    if (rest_.challengerHz > 0.0 && std::abs(f / rest_.challengerHz - 1.0) < kSteadyEnough) {
        rest_.challengerHeldSeconds += seconds;
        rest_.challengerHz += (f - rest_.challengerHz) * (seconds / rest_.challengerHeldSeconds);
    } else {
        rest_.challengerHz = f;
        rest_.challengerHeldSeconds = seconds;
    }
    const double enoughToTakeOver = rest_.hz > 0.0
        ? std::max(kChallengerMinSeconds, rest_.heldSeconds)
        : kFirstAcquireSeconds;
    if (rest_.challengerHeldSeconds > enoughToTakeOver) {
        rest_.hz = rest_.challengerHz;
        rest_.heldSeconds = rest_.challengerHeldSeconds;
        rest_.challengerHz = 0.0;
        rest_.challengerHeldSeconds = 0.0;
    }
}

void ControlVinyl::latchProvisional() {
    if (impl_->pushed < (std::int64_t) (impl_->sampleRate * 0.5)) return;
    double nearest = 1.0e9;
    int pick = -1;
    for (std::size_t i = 0; i < impl_->candidates.size(); ++i) {
        const double p = timecoder_get_pitch(&impl_->candidates[i].tc);
        if (std::abs(p) < 0.25) continue;
        const double away = std::abs(std::abs(p) - 1.0);
        if (away < nearest) { nearest = away; pick = (int) i; }
    }
    if (pick < 0 || nearest > 0.35) return;
    impl_->provisional = pick;
}

ControlVinyl::Reading ControlVinyl::read() const {
    Reading out;
    if (impl_->candidates.empty()) return out;
    const int settled = impl_->chosen >= 0 ? impl_->chosen : impl_->provisional;
    const auto& c = impl_->candidates[(std::size_t) std::max(0, settled)];
    auto& tc = const_cast<timecoder&>(c.tc);
    const double pitch = timecoder_get_pitch(&tc);
    const double nominal = timecoder_get_resolution(&tc);
    out.carrierHz = pitch * nominal;
    out.restHz = rest_.hz;
    out.speed = rest_.hz > 0.0 ? out.carrierHz / rest_.hz : pitch;
    out.toneArriving = std::abs(out.carrierHz) > 1.0;
    if (impl_->chosen >= 0) {
        const signed int pos = timecoder_get_position(&tc, nullptr);
        if (pos != -1) {
            out.located = true;
            out.positionSamples = (std::int64_t) ((double) pos / timecoder_get_resolution(&tc)
                                                  * impl_->sampleRate);
        }
    }
    return out;
}

}
