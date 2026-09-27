// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "gui/editor/LiveControls.h"
#include "hum/caps/Midi.h"
#include "hum/dsp/DspMath.h"

namespace hum::readout {

struct RingSpot {
    float x = 0.0f, y = 0.0f;
};

inline float ringRadius(int ring, int ringCount, float outer) {
    const float inner = outer * 0.3f;
    if (ringCount < 2) return outer;
    return outer - (outer - inner) * (float) ring / (float) (ringCount - 1);
}

inline RingSpot ringSpot(int step, int steps, float radius) {
    const double turn = kTwoPi * (double) step / (double) std::max(1, steps);
    return {(float) (radius * std::sin(turn)), (float) (-radius * std::cos(turn))};
}

inline float dotSize(int steps, float radius) {
    const float room = (float) (kTwoPi * radius / (double) std::max(1, steps));
    return std::clamp(room * 0.62f, 2.5f, 9.0f);
}

struct RingPicture {
    struct Ring {
        int steps = 0, now = -1;
        unsigned fired = 0;
        std::uint64_t hits = 0;
        bool operator==(const Ring& o) const {
            return steps == o.steps && now == o.now && fired == o.fired && hits == o.hits;
        }
    };
    std::vector<Ring> rings;

    bool read(ModelHost& host, const std::string& organism) {
        auto* face = live::source<RingFace>(host, organism);
        if (face == nullptr) return false;
        std::vector<Ring> fresh((size_t) face->ringCount());
        for (int r = 0; r < face->ringCount(); ++r) {
            auto& ring = fresh[(size_t) r];
            ring.steps = std::min(64, face->ringSteps(r));
            ring.now = face->ringStepNow(r);
            ring.fired = face->ringFired(r);
            for (int s = 0; s < ring.steps; ++s)
                if (face->ringHits(r, s)) ring.hits |= std::uint64_t{1} << s;
        }
        if (fresh == rings) return false;
        rings = std::move(fresh);
        return true;
    }
};

}
