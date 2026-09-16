// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include "ym3438.h"
}

#include "common/FmMods.h"
#include "common/FmVoice.h"

namespace hum {

class FmChip {
public:
    static constexpr int kChannels = 6;
    static constexpr double kMasterClock = 7670453.0;
    static constexpr double kRate = kMasterClock / 144.0;

    struct Op {
        uint8_t mult = 1, dt = 3;
        uint8_t tl = 40;
        uint8_t rs = 0, ar = 31, dr = 8, d2r = 0, rr = 10, sl = 2, ssg = 0;
    };
    struct Patch {
        uint8_t alg = 4, fb = 4;
        std::array<Op, 4> ops{};
    };
    struct Instrument {
        std::string name;
        Patch patch;
    };

    FmChip();
    void reset();

    struct FreqReg { int block, fnum; };
    static FreqReg freqRegisters(double hz);
    static double freqOf(FreqReg r) {
        return (double) r.fnum * kMasterClock / (144.0 * (double) (1 << 20))
               * (double) (1 << (r.block - 1));
    }

    static constexpr int kMaxBank = 512;

    void useFactoryBank();
    bool loadTfi(const std::string& path);
    bool loadWopn(const std::string& path);
    int patchCount() const { return (int) bank_.size(); }
    int currentSlot() const { return slot_; }
    std::string patchName(int slot) const;
    void selectPatch(int slot);
    void setMods(const FmMods& m);

    FmVoice selectedVoice() const { return voiceAt(slot_); }
    FmVoice voiceAt(int slot) const;
    void setVoice(const FmVoice& v);

    void noteOn(int midinote, int velocity, double hz);
    void noteOff(int midinote);
    void allOff();
    void bend(double semitones);
    void retune(int ch, double semitones);

    bool voiceBusy(int ch) const;
    int voiceNote(int ch) const;
    void voiceOn(int ch, int midinote, int velocity, double hz, const Patch& pt);
    void voiceOff(int ch);

    void render(float* outL, float* outR, int n);

private:
    void push(uint8_t port, uint8_t reg, uint8_t data);
    void writeChannelPatch(int ch, int velocity, const Patch& pt);
    void writeFreq(int ch, double hz);
    void writeLfo();
    void keyOn(int ch, bool on);
    const Patch& patch() const;

    ym3438_t chip_{};
    std::vector<Instrument> bank_;
    int slot_ = 0;
    FmMods mods_;
    Patch edited_;
    bool hasEdit_ = false;
    std::array<int, kChannels> chanNote_{};
    std::array<double, kChannels> chanHz_{};
    std::array<double, kChannels> chanBendSemis_{};
    double bentHz(int ch) const {
        return chanHz_[(size_t) ch] * std::pow(2.0, chanBendSemis_[(size_t) ch] / 12.0);
    }
    int next_ = 0;

    struct Write { uint8_t bus, value; };
    std::array<Write, 2048> queue_{};
    int qHead_ = 0, qTail_ = 0;
};

}
