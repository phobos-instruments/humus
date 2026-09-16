// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Var/Var.h"

#include <cmath>

#include "hum/NamedValues.h"

namespace hum {

void Var::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    syncName(params.getText("Name"));
    const float typed = (float) params.get("Value", 0.0);
    lastTyped_ = typed;
    live_.store(typed, std::memory_order_relaxed);
    primed_ = true;
}

void Var::process(const float* const*, int, float* const*, int, int, const Transport&) {
    const float typed = (float) params.get("Value", 0.0);
    if (!primed_) {
        primed_ = true;
        lastTyped_ = typed;
        live_.store(typed, std::memory_order_relaxed);
    }
    if (std::abs(typed - lastTyped_) > 0.0f) {
        lastTyped_ = typed;
        live_.store(typed, std::memory_order_relaxed);
    }
    float received = 0.0f;
    if (NamedValues::instance().take(tag(), seenStamp_, received))
        live_.store(received, std::memory_order_relaxed);
}

}
