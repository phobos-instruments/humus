// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <map>
#include <vector>

#include "core/midi/ControlMode.h"
#include "core/midi/MidiSource.h"

namespace hum {

struct ControlGuess {
    ControlType type = ControlType::Fader;
    EncoderFormat encoder = EncoderFormat::TwosComplement;
};

inline constexpr int kEncoderBand = 15;
inline constexpr int kOffsetBand = 8;

namespace guess {

inline bool within(const std::vector<int>& values, int lo, int hi) {
    return std::all_of(values.begin(), values.end(), [lo, hi](int v) { return v >= lo && v <= hi; });
}

inline bool nearOneOrTop(int v) { return (v >= 1 && v <= kEncoderBand) || v >= 128 - kEncoderBand; }
inline bool nearOneOrSign(int v) { return (v >= 1 && v <= kEncoderBand) || (v >= 65 && v <= 64 + kEncoderBand); }

inline bool repeats(const std::vector<int>& values) {
    std::map<int, int> seen;
    for (int v : values)
        if (++seen[v] >= 2) return true;
    return false;
}

inline bool buttonLike(const std::vector<int>& values) {
    int high = -1;
    for (int v : values) {
        if (v == 0) continue;
        if (v < 64 || (high >= 0 && v != high)) return false;
        high = v;
    }
    return high >= 0;
}

}

inline ControlGuess guessControl(int source, const std::vector<int>& values) {
    if (isNoteSource(source)) return {ControlType::Button, EncoderFormat::TwosComplement};
    if (!isCcSource(source) || values.empty()) return {};
    if (guess::buttonLike(values)) return {ControlType::Button, EncoderFormat::TwosComplement};
    if (!guess::repeats(values)) return {};
    const bool crossesCentre = std::find(values.begin(), values.end(), 64) != values.end();
    if (guess::within(values, 64 - kOffsetBand, 64 + kOffsetBand) && !crossesCentre)
        return {ControlType::Encoder, EncoderFormat::Offset64};
    if (std::all_of(values.begin(), values.end(), guess::nearOneOrTop))
        return {ControlType::Encoder, EncoderFormat::TwosComplement};
    if (std::all_of(values.begin(), values.end(), guess::nearOneOrSign))
        return {ControlType::Encoder, EncoderFormat::SignBit};
    return {};
}

}
