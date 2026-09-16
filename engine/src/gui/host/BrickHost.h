// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace juce {
class AudioDeviceManager;
class File;
class MidiMessage;
}

namespace hum {

class Organism;
struct PatchDocumentModel;
class AutomationHost;
class ClipEditor;
class DeckHost;
class FileHost;
class MidiHost;
class ModHost;
class OscHost;
class PatternHost;

class BrickHost {
public:
    virtual ~BrickHost() = default;

    virtual double liveParamValue(const std::string& organism, const std::string& param) = 0;
    virtual double liveParamMax(const std::string& organism, const std::string& param) = 0;
    virtual std::string liveParamText(const std::string& organism, const std::string& param) = 0;
    virtual void setParam(const std::string& organism, const std::string& param, double value) = 0;
    virtual void setParamText(const std::string& organism, const std::string& param,
                              const std::string& text) = 0;
    virtual void setParamRange(const std::string& organism, const std::string& param,
                               double min, double max) = 0;
    virtual void editParam(const std::string& organism, const std::string& param, double value) = 0;
    virtual void beginParamDrag(const std::string& organism, const std::string& param) = 0;
    virtual void endParamDrag() = 0;
    virtual bool rollLocked(const std::string& organism, const std::string& param) const = 0;
    virtual void setRollLocked(const std::string& organism, const std::string& param, bool locked) = 0;
    virtual void pushUndo() = 0;
    virtual void pushParamStep() = 0;
    virtual void beginTransaction() = 0;
    virtual void endTransaction() = 0;
    virtual void notePanelEdit(const std::string& organism) = 0;
    virtual bool bypassed(const std::string& name) const = 0;

    virtual const PatchDocumentModel& model() const = 0;
    virtual Organism* liveOrganism(const std::string& name) = 0;
    virtual const std::string& documentPath() const = 0;
    virtual juce::File documentDir() const = 0;

    virtual bool isPlaying() const = 0;
    virtual double positionBeats() = 0;
    virtual void setPositionBeats(double beat) = 0;
    virtual void play() = 0;
    virtual double tempo() const = 0;
    virtual double groove() const = 0;
    virtual std::string grooveUnit() const = 0;

    virtual bool ensureAudio() = 0;
    virtual bool audioAlive() const = 0;
    virtual int nodeMeter(const std::string& name, float* levels, int maxCh) = 0;
    virtual juce::AudioDeviceManager& audioDevices() = 0;
    virtual void injectLiveMidi(const juce::MidiMessage& m) = 0;
    virtual void injectLiveMidiToNode(const std::string& node, const juce::MidiMessage& m) = 0;
    virtual void showVisuals(const std::string& organism) = 0;
    virtual bool canShowParameterControl() const = 0;
    virtual void showParameterControl(const std::string& organism, const std::string& param) = 0;
    virtual void noteNodeRolled(const std::string& organism) = 0;

    virtual void holdPatternSync() = 0;
    virtual void releasePatternSync() = 0;

    virtual PatternHost& patterns() = 0;
    virtual ClipEditor& clips() = 0;
    virtual DeckHost& decks() = 0;
    virtual FileHost& files() = 0;
    virtual MidiHost& midi() = 0;
    virtual OscHost& osc() = 0;
    virtual ModHost& mod() = 0;
    virtual AutomationHost& automation() = 0;
};

class PatternSyncHold {
public:
    explicit PatternSyncHold(BrickHost& host) : host_(host) { host_.holdPatternSync(); }
    ~PatternSyncHold() { host_.releasePatternSync(); }
    PatternSyncHold(const PatternSyncHold&) = delete;
    PatternSyncHold& operator=(const PatternSyncHold&) = delete;

private:
    BrickHost& host_;
};

}
