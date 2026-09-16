// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "synth.h"
#include "controllers.h"
#include "dx7note.h"
#include "lfo.h"
#undef N

#include "common/FmMods.h"
#include "common/FmVoice.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class PhSixOp {
public:
    static constexpr int kVoices = 8;
    static constexpr int kBlock = 64;
    static constexpr double kRate = kDefaultSampleRate;
    static constexpr int kSlots = 32;

    PhSixOp();
    void reset();

    bool loadSyx(const std::string& path);
    void useFactoryBank();
    std::string patchName(int slot) const;
    int currentSlot() const { return currentSlot_; }
    void selectPatch(int slot);
    void setMods(const FmMods& m);

    FmVoice selectedVoice() const { return voiceAt(currentSlot_); }
    FmVoice voiceAt(int slot) const;
    void setVoice(const FmVoice& v);

    void noteOn(int midinote, int velocity, double hz);
    void noteOff(int midinote);
    void allOff();
    void setBend(double semitones);

    void renderBlock(float* out);

private:
    void rebuildVoiced();

    struct Voice {
        Dx7Note note;
        int midi = -1;
        bool gate = false;
        int quiet = 0;
    };

    std::array<std::array<uint8_t, 128>, kSlots> bank_{};
    char current_[156] = {0};
    char voiced_[156] = {0};
    char edited_[156] = {0};
    bool hasEdit_ = false;
    int currentSlot_ = -1;
    FmMods mods_;
    std::array<Voice, kVoices> voices_;
    int next_ = 0;
    Lfo lfo_;
    Controllers controllers_{};
};

}
