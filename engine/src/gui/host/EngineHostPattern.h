// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"
#include "gui/host/PatternEdits.h"
#include "hum/PatternMatrix.h"

namespace hum {

class PatternHost : public PatternEdits {
public:
    PatternHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), patternSync_(core) {}

    void ensure(const std::string& name, int laneCount = 8) override;
    void ensureBanks(const std::string& name, int laneCount, int banks) override;
    int laneOffset(const std::string& name) const override;
    std::vector<const PatternChannel*> triggerLanes(const std::string& name) const override;
    void setLaneTriggers(const std::string& name, int bank, int lane,
                         const std::vector<int>& triggers) override;
    void addTrigger(const std::string& name, int channel, int tick) override;
    void removeTrigger(const std::string& name, int channel, int tick, int tol = 0) override;
    int  moveTrigger(const std::string& name, int channel, int oldTick, int newTick) override;
    void clearChannel(const std::string& name, int channel) override;
    void setDuration(const std::string& name, int ticks) override;
    void setResolution(const std::string& name, const std::string& matrixResolution) override;
    void setChannelSnap(const std::string& name, int channel, const std::string& snap) override;
    void reframe(const std::string& name, int deltaTicks) override;
    void nudgeChannel(const std::string& name, int channel, int deltaTicks) override;

    void ensureMatrix(const std::string& name, const std::string& type, int steps,
                      const std::string& seed = {}) override;
    int bank(const std::string& name) const override;
    std::vector<BasslineStep> basslineSteps(const std::string& name) const override;
    std::vector<BasslineStep> basslineSteps(const std::string& name, int bank) const override;
    void setBasslineStep(const std::string& name, int index, const BasslineStep& s) override;
    void setBasslineSteps(const std::string& name, int bank,
                          const std::vector<BasslineStep>& steps) override;
    std::vector<ArpStep> arpSteps(const std::string& name) const override;
    void setArpStep(const std::string& name, int index, const ArpStep& s) override;
    std::vector<bool> arpUps(const std::string& name) const override;
    void setArpUp(const std::string& name, int index, bool up) override;

    void ensureNote(const std::string& name) override;
    void ensureAudio(const std::string& name) override;
    std::vector<NoteEvent> noteEvents(const std::string& name) const override;
    void setNoteEvents(const std::string& name, const std::vector<NoteEvent>& notes,
                       int durationTicks) override;

private:
    BrickHost& host_;
    HostDocument& doc_;
    HostPatterns& patternSync_;
};

}
