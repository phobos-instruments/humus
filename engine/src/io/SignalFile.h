// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum::signalfile {

enum class Kind { None, TextColumns, Biosignal, CircuitSim };

struct Signal {
    std::vector<float> samples;
    double sampleRate = 0.0;
    std::string label;
    int channels = 0;
    Kind kind = Kind::None;
};

inline constexpr int kMaxSamples = 1 << 21;
inline constexpr int kLeastSamples = 32;

const char* kindName(Kind k);

bool load(const juce::File& file, Signal& out);

bool loadTextColumns(const juce::String& text, Signal& out);
bool loadBiosignal(const juce::MemoryBlock& bytes, Signal& out);
bool loadCircuitSim(const juce::MemoryBlock& bytes, Signal& out);

}
