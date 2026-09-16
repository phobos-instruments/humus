// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"

namespace hum {


struct DeckParams {
    std::string bpm, cue, gridOffset, hotCuePrefix, loop, loopBeats, loopIn, loopOut, quantize;
    std::string hotCue(int index) const { return hotCuePrefix + std::to_string(index); }
};

class DeckHost {
public:
    DeckHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), audio_(core) {}

    std::int64_t position(const std::string& name);
    std::int64_t length(const std::string& name);
    double       sampleRate(const std::string& name);
    double       effectiveBpm(const std::string& name);
    void         seek(const std::string& name, std::int64_t sample);
    void         setBend(const std::string& name, double percent);
    void         setScrub(const std::string& name, bool active, double targetSample);
    std::vector<float> waveform(const std::string& name);

    void setCue(const std::string& name, const DeckParams& p);
    void jumpCue(const std::string& name, const DeckParams& p);
    void setHotCue(const std::string& name, const DeckParams& p, int index);
    void jumpHotCue(const std::string& name, const DeckParams& p, int index);
    void clearHotCue(const std::string& name, const DeckParams& p, int index);
    void setBeatLoop(const std::string& name, const DeckParams& p, double beats);
    void scaleBeatLoop(const std::string& name, const DeckParams& p, double factor);
    void toggleLoop(const std::string& name, const DeckParams& p);
    void beginLoopRoll(const std::string& name, const DeckParams& p, double beats);
    void endLoopRoll(const std::string& name, const DeckParams& p);

private:
    std::int64_t quantizeToGrid(const std::string& name, const DeckParams& p, std::int64_t sample);
    BrickHost& host_;
    HostDocument& doc_;
    HostGraph& audio_;
};

}
