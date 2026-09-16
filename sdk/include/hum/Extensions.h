// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>

#include "hum/Meter.h"

namespace hum {

struct TransportExt {
    std::uint32_t size = sizeof(TransportExt);
    const MeterChange* meterChanges = nullptr;
    std::int32_t meterCount = 0;
};
struct ParameterExt { std::uint32_t size = sizeof(ParameterExt); };
struct MidiEventExt { std::uint32_t size = sizeof(MidiEventExt); };
struct PatternChannelExt { std::uint32_t size = sizeof(PatternChannelExt); };
struct OrganismStateExt { std::uint32_t size = sizeof(OrganismStateExt); };

template <class Ext>
bool extHas(const Ext* ext, std::uint32_t fieldEnd) {
    return ext != nullptr && ext->size >= fieldEnd;
}

}
