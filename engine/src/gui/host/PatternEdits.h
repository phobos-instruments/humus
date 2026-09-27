// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "hum/PatternMatrix.h"

namespace hum {

class PatternEdits {
public:
    virtual ~PatternEdits() = default;

    virtual void ensure(const std::string& name, int laneCount = 8) = 0;
    virtual void ensureBanks(const std::string& name, int laneCount, int banks) = 0;
    virtual int laneOffset(const std::string& name) const = 0;
    virtual std::vector<const PatternChannel*> triggerLanes(const std::string& name) const = 0;
    virtual void setLaneTriggers(const std::string& name, int bank, int lane,
                         const std::vector<int>& triggers) = 0;
    virtual void addTrigger(const std::string& name, int channel, int tick) = 0;
    virtual void removeTrigger(const std::string& name, int channel, int tick, int tol = 0) = 0;
    virtual int  moveTrigger(const std::string& name, int channel, int oldTick, int newTick) = 0;
    virtual void clearChannel(const std::string& name, int channel) = 0;
    virtual void setDuration(const std::string& name, int ticks) = 0;
    virtual void setResolution(const std::string& name, const std::string& matrixResolution) = 0;
    virtual void setChannelSnap(const std::string& name, int channel, const std::string& snap) = 0;
    virtual void reframe(const std::string& name, int deltaTicks) = 0;
    virtual void nudgeChannel(const std::string& name, int channel, int deltaTicks) = 0;

    virtual void ensureMatrix(const std::string& name, const std::string& type, int steps,
                      const std::string& seed = {}) = 0;
    virtual int bank(const std::string& name) const = 0;
    virtual std::vector<BasslineStep> basslineSteps(const std::string& name) const = 0;
    virtual std::vector<BasslineStep> basslineSteps(const std::string& name, int bank) const = 0;
    virtual void setBasslineStep(const std::string& name, int index, const BasslineStep& s) = 0;
    virtual void setBasslineSteps(const std::string& name, int bank,
                          const std::vector<BasslineStep>& steps) = 0;
    virtual std::vector<ArpStep> arpSteps(const std::string& name) const = 0;
    virtual void setArpStep(const std::string& name, int index, const ArpStep& s) = 0;
    virtual std::vector<bool> arpUps(const std::string& name) const = 0;
    virtual void setArpUp(const std::string& name, int index, bool up) = 0;

    virtual void ensureNote(const std::string& name) = 0;
    virtual void ensureAudio(const std::string& name) = 0;
    virtual std::vector<NoteEvent> noteEvents(const std::string& name) const = 0;
    virtual void setNoteEvents(const std::string& name, const std::vector<NoteEvent>& notes,
                       int durationTicks) = 0;
};

}
