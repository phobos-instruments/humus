// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cstdint>

#include "common/GmBank.h"
#include "common/RateRing.h"

#include "common/FmChip.h"

namespace hum {

class FmVoices {
public:
    static constexpr int kMaxChips = 4;
    static constexpr int kMidiChannels = 16;
    static constexpr int kDrumChannel = 9;

    void prepare(double hostRate);
    void setChipCount(int chips);
    int chipCount() const { return chips_; }
    void setBank(const GmBank* bank) { bank_ = bank; }

    void reset();
    void programChange(int channel, int program);
    int programOf(int channel) const;
    void noteOn(int channel, int note, int velocity);
    void noteOff(int channel, int note);
    void allNotesOff(int channel);
    void pitchBend(int channel, double semitones);

    void render(float* left, float* right, int numSamples, float level);

    int sounding() const;
    int voiceCapacity() const { return chips_ * FmChip::kChannels; }

private:
    struct Slot {
        int channel = -1;
        int note = -1;
        uint64_t stamp = 0;
    };

    static bool valid(int channel) { return channel >= 0 && channel < kMidiChannels; }
    int findFree() const;
    int findOldest() const;
    void stop(int voice);

    std::array<FmChip, kMaxChips> chip_{};
    std::array<Slot, kMaxChips * FmChip::kChannels> slots_{};
    std::array<int, kMidiChannels> program_{};
    std::array<double, kMidiChannels> bendSemis_{};
    const GmBank* bank_ = nullptr;
    RateRing ring_;
    double hostRate_ = 48000.0;
    int chips_ = 4;
    uint64_t clock_ = 0;
};

}
