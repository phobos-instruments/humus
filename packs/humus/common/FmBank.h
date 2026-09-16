// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "common/FmChip.h"

namespace hum::fmbank {

const std::vector<FmChip::Instrument>& chipFactory();

struct WopnLayout {
    size_t first = 0;
    size_t stride = 0;
    int melodicBanks = 0;
    int percussionBanks = 0;
    int instruments() const { return (melodicBanks + percussionBanks) * 128; }
};

bool wopnLayout(const uint8_t* data, size_t size, WopnLayout& out);
std::string wopnName(const uint8_t* entry);
FmChip::Patch wopnPatch(const uint8_t* entry);

}
