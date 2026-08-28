#pragma once
#include <cstdint>

namespace hum {

struct PhVoiceOp {
    int level = 0;
    int ratio = 1;
    int detune = 7;
    int attack = 99;
    int decay = 40;
    int sustain = 90;
    int release = 55;

    bool operator==(const PhVoiceOp& o) const {
        return level == o.level && ratio == o.ratio && detune == o.detune
               && attack == o.attack && decay == o.decay && sustain == o.sustain
               && release == o.release;
    }
};

struct PhVoice {
    int algorithm = 1;
    int feedback = 0;
    PhVoiceOp ops[6];

    bool operator==(const PhVoice& o) const {
        if (algorithm != o.algorithm || feedback != o.feedback) return false;
        for (int i = 0; i < 6; ++i)
            if (!(ops[i] == o.ops[i])) return false;
        return true;
    }
    bool operator!=(const PhVoice& o) const { return !(*this == o); }
};

}
