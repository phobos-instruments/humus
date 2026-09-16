// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include "opl3.h"
}

#include "common/FmMods.h"
#include "common/FmVoice.h"

namespace hum {

class PhOpl {
public:
    static constexpr int kChannels = 18;
    static constexpr int kPairs = 6;
    static constexpr double kRate = 14318181.0 / 288.0;

    struct Op {
        uint8_t avekm = 0x01, ksltl = 32, ardr = 0xF4, slrr = 0x27, ws = 0;
    };
    struct Patch {
        bool fourOp = false;
        uint8_t fbcon1 = 0, fbcon2 = 0;
        int8_t noteOffset = 0;
        std::array<Op, 4> ops{};
    };
    struct Instrument {
        std::string name;
        Patch patch;
    };

    PhOpl();
    void reset();

    struct FreqReg { int block, fnum; };
    static FreqReg freqRegisters(double hz);
    static double freqOf(FreqReg r) {
        return (double) r.fnum * kRate / (double) (1 << (20 - r.block));
    }

    static constexpr int kMaxBank = 512;

    void useFactoryBank();
    bool loadWopl(const std::string& path);
    bool parseWopl(const uint8_t* d, size_t n);
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

    void render(float* outL, float* outR, int n);

private:
    static int pairOf(int ch);
    void write(uint16_t reg, uint8_t data);
    void writeOp(int ch, int opIndex, const Op& op, bool carrier, int velocity);
    void writeChannelPatch(int ch, bool secondary, int velocity);
    void writeFreq(int ch, double hz, bool on);
    const Patch& patch() const;

    opl3_chip chip_{};
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
    int next_ = 0, nextPair_ = 0;
    uint8_t fourOpMask_ = 0;
};

}
