// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>

#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/Parameter.h"
#include "hum/Pattern.h"
#include "hum/Transport.h"
#include "hum/Tuning.h"

namespace hum {

inline std::uint64_t abiShape() {
    std::uint64_t h = 0xcbf29ce484222325ull;
    for (std::uint64_t v : {sizeof(Organism), sizeof(OrganismState), sizeof(Parameter),
                            sizeof(ParameterSet), sizeof(Transport), sizeof(Tuning),
                            sizeof(Pattern), sizeof(PatternChannel), sizeof(MidiEvent),
                            sizeof(std::string), sizeof(std::function<void()>)})
        h = (h ^ v) * 0x100000001b3ull;
    return h;
}

}
