// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum {

struct AudioGlitchWindow {
    unsigned silentBlocks = 0;
    unsigned lateBlocks = 0;
    int deviceDropouts = 0;
    float loadPeak = 0.0f;
    double longestHoldMs = 0.0;
    std::string holdSite;
    int blockSize = 0;
    double sampleRate = 0.0;
};

bool hasGlitches(const AudioGlitchWindow& window);

std::string describeAudioGlitches(const AudioGlitchWindow& window);

std::string codeSite(const char* file, int line);

}
