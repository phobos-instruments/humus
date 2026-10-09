// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/audio/AudioGlitches.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include "hum/Number.h"

namespace hum {

namespace {

std::string counted(unsigned n, const char* one, const char* many, const char* why) {
    return std::to_string(n) + " " + (n == 1 ? one : many) + " (" + why + ")";
}

std::string oneDecimal(double v) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.1f", v);
    fixDecimalPoint(text);
    return text;
}

}

bool hasGlitches(const AudioGlitchWindow& window) {
    return window.silentBlocks > 0 || window.lateBlocks > 0 || window.deviceDropouts > 0;
}

std::string describeAudioGlitches(const AudioGlitchWindow& window) {
    std::vector<std::string> parts;
    if (window.silentBlocks > 0)
        parts.push_back(counted(window.silentBlocks, "silent block", "silent blocks", "the audio lock was busy"));
    if (window.lateBlocks > 0)
        parts.push_back(counted(window.lateBlocks, "late block", "late blocks",
                                "the patch took longer than the buffer"));
    if (window.deviceDropouts > 0)
        parts.push_back(counted((unsigned) window.deviceDropouts, "device dropout", "device dropouts",
                                "the interface or its driver missed a deadline"));
    std::string out = "audio glitch: ";
    for (size_t i = 0; i < parts.size(); ++i) out += (i > 0 ? ", " : "") + parts[i];
    out += "; peak load " + std::to_string((int) std::lround(window.loadPeak * 100.0f)) + "%";
    if (window.blockSize > 0 && window.sampleRate > 0.0)
        out += "; " + std::to_string(window.blockSize) + " samples at "
             + std::to_string((long) std::lround(window.sampleRate)) + " Hz";
    if (window.longestHoldMs > 0.0 && !window.holdSite.empty())
        out += "; longest audio-lock hold " + oneDecimal(window.longestHoldMs) + " ms at " + window.holdSite;
    return out;
}

std::string codeSite(const char* file, int line) {
    if (file == nullptr || *file == '\0') return {};
    std::string name(file);
    if (const auto slash = name.find_last_of("/\\"); slash != std::string::npos) name = name.substr(slash + 1);
    return name + ":" + std::to_string(line);
}

}
