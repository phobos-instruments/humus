// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Scope/Scope.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "hum/dsp/DspMath.h"

namespace hum {

void Scope::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    left_.assign((size_t) kRingSamples, 0.0f);
    right_.assign((size_t) kRingSamples, 0.0f);
    reset();
}

void Scope::reset() {
    std::fill(left_.begin(), left_.end(), 0.0f);
    std::fill(right_.begin(), right_.end(), 0.0f);
    written_.store(0, std::memory_order_relaxed);
}

int Scope::sweepSamples() const {
    const double seconds = params.get("Time", 20.0) * 0.001;
    return std::clamp((int) std::lround(seconds * sampleRate_), 16, kRingSamples / 2);
}

void Scope::process(const float* const* in, int numIn, float* const*, int,
                    int numSamples, const Transport&) {
    const float* l = numIn > 0 && in ? in[0] : nullptr;
    const float* r = numIn > 1 && in && in[1] ? in[1] : l;
    window_.store(sweepSamples(), std::memory_order_relaxed);
    if (params.get("Hold", 0.0) >= 0.5 || left_.empty()) return;

    std::int64_t w = written_.load(std::memory_order_relaxed);
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        const float a = l ? l[i] : 0.0f;
        const float b = r ? r[i] : 0.0f;
        left_[(size_t) (w & kRingMask)] = a;
        right_[(size_t) (w & kRingMask)] = b;
        ++w;
        peak = std::max(peak, std::max(std::abs(a), std::abs(b)));
    }
    written_.store(w, std::memory_order_relaxed);
    if (peak > 1.0e-6f) stamp_.fetch_add(1, std::memory_order_relaxed);
}

TraceSource::Shape Scope::traceShape() const {
    Shape shape;
    const int mode = (int) std::lround(params.get("Mode", 0.0));
    shape.windowSamples = window_.load(std::memory_order_relaxed);
    shape.plotsLeftAgainstRight = mode == kLeftAgainstRight;
    shape.sumsToMono = mode == kMono;
    shape.held = params.get("Hold", 0.0) >= 0.5;
    shape.gain = (float) params.get("Zoom", 1.0);
    return shape;
}

int Scope::traceRead(float* left, float* right, int samples) const {
    if (left_.empty()) return 0;
    const std::int64_t end = written_.load(std::memory_order_relaxed);
    const int n = (int) std::clamp<std::int64_t>(std::min<std::int64_t>(samples, end), 0, kRingSamples);
    for (int i = 0; i < n; ++i) {
        const auto at = (size_t) ((end - n + i) & kRingMask);
        left[i] = left_[at];
        right[i] = right_[at];
    }
    return n;
}

}
