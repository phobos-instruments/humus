// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "gui/host/BrickHost.h"

namespace juce { template <typename> class Point; }

namespace hum {

class RecordHost;

class TracksHost : public virtual BrickHost {
public:
    ~TracksHost() override = default;

    virtual unsigned locateStamp() const = 0;
    virtual double locateBeat() const = 0;
    virtual void markDirty() = 0;
    virtual void removeOrganism(const std::string& name) = 0;
    virtual bool renameOrganism(const std::string& oldName, const std::string& newName) = 0;
    virtual void setSoloed(const std::string& name, bool on) = 0;
    virtual bool soloed(const std::string& name) const = 0;
    virtual void setSoloable(std::vector<std::string> rows) = 0;
    virtual void setTrackMuted(const std::string& name, bool on) = 0;
    virtual bool trackMuted(const std::string& name) const = 0;
    virtual void connectMidi(const std::string& src, int srcPort, const std::string& dst, int dstPort) = 0;
    virtual void removeMidiConnection(const std::string& src, int srcPort, const std::string& dst, int dstPort) = 0;
    virtual void setLiveTargets(std::vector<std::string> nodes) = 0;
    virtual double songEndBeat() const = 0;
    virtual bool nodeArrangesVideo(const std::string& name) = 0;
    virtual int midiInletsOf(const std::string& name) = 0;
    virtual int midiOutletsOf(const std::string& name) = 0;
    virtual std::vector<std::string> arrangeableNodes() = 0;
    virtual bool nodeRecordsAudio(const std::string& name) = 0;
    virtual bool nodeRecordsMedia(const std::string& name) = 0;
    virtual std::vector<std::pair<int, std::string>> choiceItems(const std::string& source,
                                                                 const std::string& organism = {}) = 0;
    virtual std::string bounceSourceOf(const std::string& node) = 0;
    virtual PatchDocumentModel& model() = 0;
    const PatchDocumentModel& model() const override = 0;
    virtual juce::Point<int> spotBelowPatch() const = 0;
    virtual std::string addOrganism(const std::string& className, juce::Point<int> at,
                               const std::string& podScope = {}) = 0;
    virtual RecordHost& record() = 0;
    virtual void setSongLengthBeats(double beats) = 0;
    virtual int connectToMaster(const std::string& node) = 0;
    virtual double sampleRate() const = 0;
    virtual std::string consolidate(const std::string& node, double fromBeat, double toBeat,
                            std::string& error) = 0;
    virtual std::string printToTimeline(const std::string& node, std::string& error, int atTick = -1,
                                const std::string& preferredTarget = {}) = 0;
};

}
