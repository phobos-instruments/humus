// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

namespace hum {

inline constexpr int kMp3RateSteps = 4;
inline constexpr int kMp3RateBest = 0;
inline constexpr int kMp3VbrQuality[kMp3RateSteps] = {0, 2, 4, 5};
inline constexpr int kMp3KbitPerSecond[kMp3RateSteps] = {245, 190, 165, 130};

bool writeMp3(const std::string& path,
              const std::vector<std::vector<float>>& channels,
              double sampleRate, int rate = kMp3RateBest);

double mp3BytesPerSecond(int rate);

}
