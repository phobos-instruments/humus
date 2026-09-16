// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"
#include "hum/PatternMatrix.h"

namespace hum {


class PatternHost {
public:
    PatternHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), patternSync_(core) {}

    void ensure(const std::string& name, int laneCount = 8);
    void ensureBanks(const std::string& name, int laneCount, int banks);
    int laneOffset(const std::string& name) const;
    std::vector<const PatternChannel*> triggerLanes(const std::string& name) const;
    void setLaneTriggers(const std::string& name, int bank, int lane,
                         const std::vector<int>& triggers);
    void addTrigger(const std::string& name, int channel, int tick);
    void removeTrigger(const std::string& name, int channel, int tick, int tol = 0);
    int  moveTrigger(const std::string& name, int channel, int oldTick, int newTick);
    void clearChannel(const std::string& name, int channel);
    void setDuration(const std::string& name, int ticks);
    void setResolution(const std::string& name, const std::string& matrixResolution);
    void setChannelSnap(const std::string& name, int channel, const std::string& snap);
    void reframe(const std::string& name, int deltaTicks);
    void nudgeChannel(const std::string& name, int channel, int deltaTicks);

    void ensureMatrix(const std::string& name, const std::string& type, int steps,
                      const std::string& seed = {});
    int bank(const std::string& name) const;
    std::vector<BasslineStep> basslineSteps(const std::string& name) const;
    std::vector<BasslineStep> basslineSteps(const std::string& name, int bank) const;
    void setBasslineStep(const std::string& name, int index, const BasslineStep& s);
    void setBasslineSteps(const std::string& name, int bank,
                          const std::vector<BasslineStep>& steps);
    std::vector<ArpStep> arpSteps(const std::string& name) const;
    void setArpStep(const std::string& name, int index, const ArpStep& s);
    std::vector<bool> arpUps(const std::string& name) const;
    void setArpUp(const std::string& name, int index, bool up);

    void ensureNote(const std::string& name);
    void ensureAudio(const std::string& name);
    std::vector<NoteEvent> noteEvents(const std::string& name) const;
    void setNoteEvents(const std::string& name, const std::vector<NoteEvent>& notes,
                       int durationTicks);

private:
    BrickHost& host_;
    HostDocument& doc_;
    HostPatterns& patternSync_;
};

}
