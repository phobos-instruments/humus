// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

namespace hum {

bool writeWav(const std::string& path,
              const std::vector<std::vector<float>>& channels,
              double sampleRate, int bitsPerSample = 24);

bool writeSound(const std::string& path,
                const std::vector<std::vector<float>>& channels,
                double sampleRate, int bitsPerSample = 24, int mp3Rate = 0);

}
