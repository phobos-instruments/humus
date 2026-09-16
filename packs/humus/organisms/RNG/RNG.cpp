// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "RNG/RNG.h"

#include <algorithm>

#include "RNG/Entropy.h"
#include "common/NumberFormat.h"

namespace hum {

void RNG::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
    state_ = rng::osEntropy();
    due_ = 0.0;
    pending_ = true;
    triggerHeld_ = false;
    format_.store((int) params.get("Format", 0.0), std::memory_order_relaxed);
}

void RNG::process(const float* const* in, int numIn,
                  float* const* out, int numOut,
                  int numSamples, const Transport& transport) {
    (void) in; (void) numIn; (void) out; (void) numOut;

    const int format = (int) params.get("Format", 0.0);
    format_.store(format, std::memory_order_relaxed);

    const bool held = params.get("Trigger", 0.0) >= 0.5;
    if (held && !triggerHeld_) pending_ = true;
    triggerHeld_ = held;

    const bool sync = params.get("Sync", 0.0) >= 0.5;
    const double every = sync
        ? std::max(1.0, transport.samplesPerBeat() * std::max(0.0625, params.get("SyncBeats", 1.0)))
        : (params.get("Rate", 2.0) > 0.0 ? sampleRate_ / params.get("Rate", 2.0) : 0.0);
    if (every > 0.0) {
        due_ -= (double) numSamples;
        if (due_ <= 0.0) {
            due_ = every;
            pending_ = true;
        }
    }

    if (pending_) {
        pending_ = false;
        value_.store(numfmt::spreadAcross(rng::unitFrom(state_), format),
                     std::memory_order_relaxed);
    }
}

}
