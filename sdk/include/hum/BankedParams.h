// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstddef>
#include <utility>

#include "hum/ParamBanks.h"
#include "hum/ParamRef.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum {

template <std::size_t N>
class BankedParams {
public:
    explicit BankedParams(std::array<ParamRef, N> refs) : refs_(std::move(refs)) {}

    void read(const Pattern& p) {
        for (int b = 0; b < kPatternBanks; ++b) {
            const auto at = (std::size_t) b;
            has_[at].fill(false);
            const auto* ch = matrixChannel(p, kParamBankChannel, b);
            if (ch == nullptr || ch->matrix.empty()) continue;
            for (const auto& [name, value] : decodeBankValues(ch->matrix))
                for (std::size_t i = 0; i < N; ++i)
                    if (refs_[i].name() == name) {
                        values_[at][i] = (double) value;
                        has_[at][i] = true;
                    }
        }
    }

    double get(const ParameterSet& ps, int bank, std::size_t i, double fallback) const {
        if (bank >= 0 && bank < kPatternBanks && has_[(std::size_t) bank][i]) return values_[(std::size_t) bank][i];
        return refs_[i].get(ps, fallback);
    }

private:
    std::array<ParamRef, N> refs_;
    std::array<std::array<double, N>, kPatternBanks> values_{};
    std::array<std::array<bool, N>, kPatternBanks> has_{};
};

}
