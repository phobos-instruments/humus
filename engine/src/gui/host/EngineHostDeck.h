// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/DeckEdits.h"
#include "gui/host/HostCore.h"

namespace hum {

class DeckHost : public DeckEdits {
public:
    DeckHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), audio_(core) {}

    std::int64_t position(const std::string& name) override;
    std::int64_t length(const std::string& name) override;
    double       sampleRate(const std::string& name) override;
    double       effectiveBpm(const std::string& name) override;
    void         seek(const std::string& name, std::int64_t sample) override;
    void         setBend(const std::string& name, double percent) override;
    void         setScrub(const std::string& name, bool active, double targetSample) override;
    std::vector<float> waveform(const std::string& name) override;
    std::vector<float> waveBetween(const std::string& name, std::int64_t from, std::int64_t to,
                                   int buckets) override;

    void setCue(const std::string& name, const DeckParams& p) override;
    void jumpCue(const std::string& name, const DeckParams& p) override;
    void startPlaying(const std::string& name);
    void setHotCue(const std::string& name, const DeckParams& p, int index) override;
    void jumpHotCue(const std::string& name, const DeckParams& p, int index) override;
    void clearHotCue(const std::string& name, const DeckParams& p, int index) override;
    void nudgeHotCue(const std::string& name, const DeckParams& p, double way);
    void setBeatLoop(const std::string& name, const DeckParams& p, double beats) override;
    void scaleBeatLoop(const std::string& name, const DeckParams& p, double factor) override;
    void toggleLoop(const std::string& name, const DeckParams& p) override;
    void beginLoopRoll(const std::string& name, const DeckParams& p, double beats) override;
    void endLoopRoll(const std::string& name, const DeckParams& p) override;

    bool fireTrigger(const std::string& name, const std::string& param, double value);

private:
    std::map<std::string, int> lastPad_;
    std::int64_t quantizeToGrid(const std::string& name, const DeckParams& p, std::int64_t sample);
    BrickHost& host_;
    HostDocument& doc_;
    HostGraph& audio_;
};

}
