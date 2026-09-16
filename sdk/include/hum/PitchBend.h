// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>

#include "hum/caps/Midi.h"
#include "hum/Parameter.h"

namespace hum {

inline constexpr const char* kBendRangeParam = "BendRange";
inline constexpr double kDefaultBendRange = 2.0;

inline double bendRangeOf(const ParameterSet& params) {
    const double range = params.get(kBendRangeParam, kDefaultBendRange);
    return range < 0.0 ? 0.0 : range > 48.0 ? 48.0 : range;
}

struct PitchBend {
    static constexpr int kChannels = 16;
    static constexpr int kCentre = 8192;
    static constexpr int kMax = 16383;

    std::array<int, kChannels> wheel{};
    int lastChannel = 0;

    PitchBend() { reset(); }

    void reset() {
        wheel.fill(kCentre);
        lastChannel = 0;
    }

    static bool isBend(const MidiEvent& e) {
        return e.size >= 3 && (e.data[0] & 0xF0) == 0xE0;
    }

    bool apply(const MidiEvent& e) {
        if (!isBend(e)) return false;
        lastChannel = e.data[0] & 0x0F;
        wheel[(size_t) lastChannel] = (e.data[1] & 0x7F) | ((e.data[2] & 0x7F) << 7);
        return true;
    }

    double semitones(int channel, double rangeSemitones) const {
        const int w = wheel[(size_t) (channel & 0x0F)];
        return (double) (w - kCentre) / (double) kCentre * rangeSemitones;
    }
    double semitones(double rangeSemitones) const { return semitones(lastChannel, rangeSemitones); }

    double ratio(int channel, double rangeSemitones) const {
        return std::pow(2.0, semitones(channel, rangeSemitones) / 12.0);
    }
    double ratio(double rangeSemitones) const { return ratio(lastChannel, rangeSemitones); }
};

}
