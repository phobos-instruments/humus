#pragma once
#include <string>
#include <vector>

#include "hum/PatternMatrix.h"

namespace hum {

class EngineHost;

class PatternHost {
public:
    explicit PatternHost(EngineHost& host) : host_(host) {}

    void ensure(const std::string& name, int laneCount = 8);
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
    std::vector<BasslineStep> basslineSteps(const std::string& name) const;
    void setBasslineStep(const std::string& name, int index, const BasslineStep& s);
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
    EngineHost& host_;
};

}
