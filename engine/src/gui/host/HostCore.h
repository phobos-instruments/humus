// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <juce_graphics/juce_graphics.h>

namespace juce {
class MidiInputCallback;
class MidiOutput;
}

namespace hum {

class AudioGraph;
class Recorder;
class RecordHost;
struct OrganismModel;
struct PatchDocumentModel;

enum class BeforeRecord { None, CountIn, PreRoll };

class HostDocument {
public:
    virtual PatchDocumentModel& document() = 0;
    virtual OrganismModel* mutableByName(const std::string& name) = 0;
    virtual void flagDirty() = 0;
    virtual void markDirty() = 0;
    virtual void bumpChangeStamp() = 0;
    virtual void bumpLiveControl() = 0;
    virtual void pokeLiveRefresh() = 0;
    virtual bool setLiveControl(bool on) = 0;
    virtual bool setDerivedControl(bool on) = 0;
    virtual bool derivedControl() const = 0;
    virtual void noteTopologyChanged() = 0;

protected:
    ~HostDocument() = default;
};

class HostPatterns {
public:
    virtual void markPatternEdited() = 0;
    virtual void recordPatternRevert(const std::string& organism) = 0;
    virtual void syncPattern(const std::string& name) = 0;

protected:
    ~HostPatterns() = default;
};

class HostGraph {
public:
    virtual AudioGraph* graph() = 0;
    virtual juce::CriticalSection& graphLock() = 0;
    virtual double sampleRate() const = 0;
    virtual void setSyncOutput(juce::MidiOutput* out) = 0;
    virtual juce::MidiInputCallback& midiInputSink() = 0;

protected:
    ~HostGraph() = default;
};

class HostNodes {
public:
    virtual std::string addOrganism(const std::string& className, juce::Point<int> at,
                                    const std::string& podScope = {}) = 0;
    virtual void removeOrganism(const std::string& name) = 0;
    virtual std::string replaceOrganism(const std::string& name, const std::string& newClass) = 0;
    virtual void setNodeInternal(const std::string& name, bool internal) = 0;
    virtual void syncNodeTrack(const std::string& name) = 0;
    virtual void pullVoiceTexts(const std::string& name) = 0;
    virtual void syncRoutes() = 0;
    virtual std::string metapadNodeName() = 0;
    virtual std::string metapadNodeNameIfAny() const = 0;
    virtual std::string clockNodeName() = 0;
    virtual std::string clockNodeNameIfAny() const = 0;
    virtual bool nodeArrangesVideo(const std::string& name) = 0;
    virtual bool nodeRecordsAudio(const std::string& name) = 0;
    virtual bool nodeRecordsVideo(const std::string& name) = 0;
    virtual std::vector<std::string> arrangeableNodes() = 0;
    virtual std::string videoSourceInto(const std::string& dst, int dstPort) const = 0;
    virtual std::string noteCaptureTarget() const = 0;

protected:
    ~HostNodes() = default;
};

class HostCapture {
public:
    using Touches = std::set<std::pair<std::string, std::string>>;

    virtual bool capturing() const = 0;
    virtual void setCapturing(bool on) = 0;
    virtual void beginCapturePass() = 0;
    virtual void endCapturePasses() = 0;
    virtual Touches& touches() = 0;
    virtual void clearTouches() = 0;
    virtual void bumpLaneStamp() = 0;
    virtual void publishClock() = 0;
    virtual void syncAutomation() = 0;
    virtual void syncMeter() = 0;

protected:
    ~HostCapture() = default;
};

class HostRecording {
public:
    virtual RecordHost& recorder() = 0;
    virtual const std::vector<std::pair<std::string, Recorder*>>& recorders() const = 0;
    virtual void stop() = 0;
    virtual void flushMidiRecording(bool finalize = false) = 0;
    virtual BeforeRecord beforeRecord() const = 0;
    virtual int beforeRecordBars() const = 0;
    virtual void armCountIn(int bars) = 0;
    virtual void armPreRoll(double punchBeat) = 0;
    virtual void cancelPreRoll() = 0;
    virtual bool preRolling() const = 0;

protected:
    ~HostRecording() = default;
};

class HostCore : public HostDocument, public HostPatterns, public HostGraph, public HostNodes,
                 public HostCapture, public HostRecording {
protected:
    ~HostCore() = default;
};

class LiveControlHold {
public:
    explicit LiveControlHold(HostDocument& doc) : doc_(doc), prev_(doc.setLiveControl(true)) {}
    ~LiveControlHold() { doc_.setLiveControl(prev_); }
    LiveControlHold(const LiveControlHold&) = delete;
    LiveControlHold& operator=(const LiveControlHold&) = delete;

private:
    HostDocument& doc_;
    bool prev_;
};

class DerivedControlHold {
public:
    explicit DerivedControlHold(HostDocument& doc) : doc_(doc), prev_(doc.setDerivedControl(true)) {}
    ~DerivedControlHold() { doc_.setDerivedControl(prev_); }
    DerivedControlHold(const DerivedControlHold&) = delete;
    DerivedControlHold& operator=(const DerivedControlHold&) = delete;

private:
    HostDocument& doc_;
    bool prev_;
};

}
