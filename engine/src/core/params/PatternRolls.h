// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <iterator>
#include <vector>

#include "core/params/Dice.h"
#include "hum/PatternMatrix.h"
#include "hum/dsp/DspMath.h"

namespace hum {

inline int basslineRoot(const std::vector<BasslineStep>& steps) {
    constexpr int kFallbackRoot = 45;
    constexpr int kHighestRoot = kBasslineHighNote - 24;
    std::array<int, kMidiMax + 1> count{};
    for (const auto& s : steps)
        if (s.gate && s.note >= 0 && s.note <= kMidiMax) ++count[(size_t) s.note];
    int root = -1;
    for (int n = 0; n <= kMidiMax; ++n)
        if (count[(size_t) n] > (root < 0 ? 0 : count[(size_t) root])) root = n;
    return root < 0 ? kFallbackRoot : std::clamp(root, kBasslineLowNote, kHighestRoot);
}

inline std::vector<BasslineStep> randomBassline(int steps, int rootNote, Dice& dice) {
    static const int kScale[] = {-5, 0, 0, 0, 3, 5, 7, 10, 12};
    std::vector<BasslineStep> out((size_t) std::max(1, steps));
    for (size_t i = 0; i < out.size(); ++i) {
        auto& s = out[i];
        s.gate = i == 0 || dice.nextDouble() < 0.65;
        int n = rootNote + kScale[dice.nextInt((int) std::size(kScale))];
        if (n - 12 >= kBasslineLowNote && dice.nextDouble() < 0.12) n -= 12;
        while (n < kBasslineLowNote) n += 12;
        while (n > kBasslineHighNote) n -= 12;
        s.note = n;
        s.accent = s.gate && dice.nextDouble() < 0.25;
        s.slide = s.gate && dice.nextDouble() < 0.20;
    }
    return out;
}

inline std::vector<bool> randomTriggerRow(int steps, double density, Dice& dice) {
    std::vector<bool> out((size_t) std::max(0, steps), false);
    if (out.empty() || density <= 0.0) return out;
    bool any = false;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = dice.nextDouble() < density;
        any = any || out[i];
    }
    if (!any) out[(size_t) dice.nextInt((int) out.size())] = true;
    return out;
}

}
