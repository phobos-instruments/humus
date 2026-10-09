// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "hum/dsp/LoudSpots.h"

namespace hum {

struct DeckSummary {
    std::vector<float> peaks;
    LoudnessScan loud;
};

struct DeckAudio {
    const float* const* channels = nullptr;
    int numChannels = 0;
    std::int64_t length = 0;
    double sampleRate = 0.0;
};

DeckSummary summaryFor(const std::string& stamp, const DeckAudio& audio, int buckets);

}
