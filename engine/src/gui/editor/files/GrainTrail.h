// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

#include "hum/caps/Samples.h"

namespace hum::files {

inline constexpr double kGlowShortest = 0.12;
inline constexpr double kGlowLongest = 1.5;
inline constexpr double kGlowTail = 0.15;

struct GrainGlow {
    float x = 0.0f, y = 0.0f;
    int file = 0;
    float life = 0.0f;
};

class GrainTrail {
public:
    static double lifetime(float grainSeconds) {
        return std::clamp((double) grainSeconds, kGlowShortest, kGlowLongest) + kGlowTail;
    }

    void take(const GrainFlashSource::Flash* flashes, int count, double now) {
        std::uint32_t newest = newest_;
        for (int i = 0; i < count; ++i) {
            const auto& f = flashes[i];
            if (primed_ && isNewer(f.serial, newest_)) seen_.push_back({f, now});
            if (isNewer(f.serial, newest)) newest = f.serial;
        }
        newest_ = newest;
        primed_ = true;
        seen_.erase(std::remove_if(seen_.begin(), seen_.end(),
                                   [now](const Seen& s) { return now - s.born >= lifetime(s.flash.seconds); }),
                    seen_.end());
    }

    std::vector<GrainGlow> glows(double now) const {
        std::vector<GrainGlow> out;
        out.reserve(seen_.size());
        for (const auto& s : seen_) {
            const double age = std::max(0.0, now - s.born), span = lifetime(s.flash.seconds);
            if (age < span) out.push_back({s.flash.x, s.flash.y, s.flash.file, (float) (1.0 - age / span)});
        }
        return out;
    }

    void forget() {
        seen_.clear();
        primed_ = false;
    }

private:
    static bool isNewer(std::uint32_t serial, std::uint32_t than) {
        return serial != 0 && (std::int32_t) (serial - than) > 0;
    }

    struct Seen {
        GrainFlashSource::Flash flash;
        double born = 0.0;
    };
    std::vector<Seen> seen_;
    std::uint32_t newest_ = 0;
    bool primed_ = false;
};

}
