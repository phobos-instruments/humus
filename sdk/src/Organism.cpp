// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/Organism.h"

namespace hum {

void Organism::loadFrom(const OrganismState& state) {
    for (const auto& p : state.properties)
        params.add(p);
}

}
