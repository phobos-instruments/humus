#pragma once
#include <string>

#include <juce_audio_basics/juce_audio_basics.h>

namespace hum {

struct KeyEstimate {
    int    pitchClass = -1;
    bool   major = true;
    double confidence = 0.0;
    std::string name;
    std::string camelot;
};

KeyEstimate detectKey(const juce::AudioBuffer<float>& buf, double sampleRate);

}
