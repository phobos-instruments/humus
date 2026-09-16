// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>

namespace hum {

struct FmVoiceOp {
    int level = 0;
    int ratio = 1;
    int detune = 7;
    int attack = 99;
    int decay = 40;
    int sustain = 90;
    int release = 55;

    bool operator==(const FmVoiceOp& o) const {
        return level == o.level && ratio == o.ratio && detune == o.detune
               && attack == o.attack && decay == o.decay && sustain == o.sustain
               && release == o.release;
    }
};

struct FmVoice {
    int algorithm = 1;
    int feedback = 0;
    FmVoiceOp ops[6];

    bool operator==(const FmVoice& o) const {
        if (algorithm != o.algorithm || feedback != o.feedback) return false;
        for (int i = 0; i < 6; ++i)
            if (!(ops[i] == o.ops[i])) return false;
        return true;
    }
    bool operator!=(const FmVoice& o) const { return !(*this == o); }
};

}
