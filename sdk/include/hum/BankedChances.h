// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "hum/ParamBanks.h"
#include "hum/ParamRef.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"
#include "hum/StepChance.h"

namespace hum {

class BankedChances {
public:
    static constexpr int kSlots = 16;

    explicit BankedChances(std::string prefix) : prefix_(std::move(prefix)), refs_(numberedParams<kSlots>(prefix_)) {}

    void read(const Pattern& p) {
        for (int b = 0; b < kPatternBanks; ++b) {
            const auto at = (size_t) b;
            const auto* ch = matrixChannel(p, kParamBankChannel, b);
            stored_[at] = ch != nullptr && !ch->matrix.empty();
            banks_[at].fill(kFullChance);
            if (!stored_[at]) continue;
            for (const auto& [name, value] : decodeBankValues(ch->matrix))
                if (const int slot = numberedSlot(name, prefix_); slot >= 0 && slot < kSlots)
                    banks_[at][(size_t) slot] = (int) std::clamp(value, 0LL, (long long) kFullChance);
        }
    }

    int at(const ParameterSet& params, int bank, long step) const {
        if (step < 0 || step >= kSlots) return kFullChance;
        if (bank >= 0 && bank < kPatternBanks && stored_[(size_t) bank]) return banks_[(size_t) bank][(size_t) step];
        return (int) std::lround(refs_[(size_t) step].get(params, (double) kFullChance));
    }

private:
    std::string prefix_;
    std::array<ParamRef, kSlots> refs_;
    std::array<std::array<int, kSlots>, kPatternBanks> banks_{};
    std::array<bool, kPatternBanks> stored_{};
};

}
