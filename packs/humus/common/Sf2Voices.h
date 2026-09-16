// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cstdint>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/dsp/Sf2File.h"

namespace hum {

class Sf2Voices {
public:
    static constexpr int kMidiChannels = 16;
    static constexpr int kDrumChannel = 9;
    static constexpr int kDrumBank = 128;
    static constexpr int kVoices = 64;
    static constexpr int kLayers = 4;

    bool load(const juce::File& file);
    bool ready() const { return data_.parsed && !data_.presets.empty(); }
    const std::string& name() const { return data_.name; }
    std::string programName(int program) const;

    void prepare(double hostRate);
    void reset();
    void programChange(int channel, int program);
    int programOf(int channel) const;
    void noteOn(int channel, int note, int velocity);
    void noteOff(int channel, int note);
    void allNotesOff(int channel);
    void pitchBend(int channel, double semitones);

    void render(float* left, float* right, int numSamples, float level);

    int sounding() const;
    int voiceCapacity() const { return kVoices; }

private:
    struct Voice {
        const sf2::Zone* zone = nullptr;
        int channel = -1;
        int note = -1;
        double pos = 0.0;
        double step = 1.0;
        double bend = 1.0;
        float peakL = 0.0f, peakR = 0.0f;
        float env = 0.0f;
        float attack = 1.0f, decay = 1.0f, sustain = 1.0f, release = 1.0f;
        bool held = false;
        bool looping = false;
        std::uint32_t start = 0, end = 0, loopStart = 0, loopEnd = 0;
        std::uint64_t stamp = 0;
    };

    static bool valid(int channel) { return channel >= 0 && channel < kMidiChannels; }
    const sf2::Preset* presetFor(int channel) const;
    int take();
    void start(const sf2::Zone& z, int channel, int note, int velocity);
    float sampleAt(const Voice& v, double pos) const;

    sf2::Sf2Data data_;
    std::array<Voice, kVoices> voices_{};
    std::array<int, kMidiChannels> program_{};
    std::array<double, kMidiChannels> bendSemis_{};
    double hostRate_ = 48000.0;
    std::uint64_t clock_ = 0;
};

}
