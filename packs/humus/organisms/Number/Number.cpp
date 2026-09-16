// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Number/Number.h"

#include "common/NumberFormat.h"

namespace hum {

void Number::prepare(double, int) {
    const int format = (int) params.get("Format", 0.0);
    format_.store(format, std::memory_order_relaxed);
    if (const auto* pv = params.byName("Value"))
        live_.store(numfmt::heldIn(pv->value, format), std::memory_order_relaxed);
}

void Number::process(const float* const*, int, float* const*, int, int, const Transport&) {
    const int format = (int) params.get("Format", 0.0);
    format_.store(format, std::memory_order_relaxed);
    const auto* pv = params.byName("Value");
    live_.store(numfmt::heldIn(pv ? pv->value : 0.0, format), std::memory_order_relaxed);
}

}
