#pragma once
#include <cstdint>

#include <juce_audio_basics/juce_audio_basics.h>

namespace hum {

struct BeatEstimate {
    double  bpm = 120.0;
    int64_t offsetSamples = 0;
    double  confidence = 0.0;
};

BeatEstimate detectBeat(const juce::AudioBuffer<float>& buf, double sampleRate,
                        double minBpm = 70.0, double maxBpm = 180.0);

}
