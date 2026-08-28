#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace hum {

struct PatternTimeSig {
    int tick = 0;
    int numerator = 4;
    int denominator = 4;
};

struct PatternChannel {
    std::string type = "trigger-timepoints";
    std::vector<int> triggers;
    std::vector<PatternTimeSig> timeSignatures;
    std::string snap;
    double swing = -1.0;
    std::string matrix;

    int startTick = -1;
    int lengthTicks = 0;
    bool loopClip = false;
    std::string name;
    int color = 0;

    std::string audioFile;
    std::int64_t audioOffset = 0;
    double audioGain = 1.0;

    int id = 0;

    enum class Warp { Off = 0, Beats = 1, Tone = 2 };
    double sourceBpm = 0.0;
    int warpMode = 0;

    int fadeInTicks = 0;
    int fadeOutTicks = 0;

    bool audioReverse = false;
    double audioPitch = 0.0;
};

struct Pattern {
    static constexpr int kTicksPerBeat = 48;

    bool present = false;
    int duration = 0;
    std::string matrixResolution = "1/16";
    std::vector<PatternChannel> channels;

    std::vector<const PatternChannel*> triggerChannels() const {
        std::vector<const PatternChannel*> out;
        for (auto& ch : channels)
            if (ch.type != "time-signatures") out.push_back(&ch);
        return out;
    }
};

}
