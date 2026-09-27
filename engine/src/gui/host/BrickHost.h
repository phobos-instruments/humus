// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/params/RangeEnd.h"
#include "gui/host/AutomationEdits.h"
#include "gui/host/ClipEdits.h"
#include "gui/host/DeckEdits.h"
#include "gui/host/FileEdits.h"
#include "gui/host/MidiEdits.h"
#include "gui/host/ModEdits.h"
#include "gui/host/OscEdits.h"
#include "gui/host/ModelHost.h"
#include "gui/host/PatternEdits.h"
#include "hum/caps/Midi.h"

namespace hum {

class BrickHost : public virtual ModelHost {
public:
    ~BrickHost() override = default;

    virtual double liveParamMax(const std::string& organism, const std::string& param) = 0;
    virtual void setParamRange(const std::string& organism, const std::string& param,
                               double min, double max) = 0;
    virtual void setRollLocked(const std::string& organism, const std::string& param, bool locked) = 0;
    virtual RangeMode rangeMode(const std::string& organism, const std::string& param) const = 0;
    virtual void setRangeMode(const std::string& organism, const std::string& param, RangeMode mode) = 0;
    virtual void pushUndo() = 0;
    virtual void pushParamStep() = 0;
    virtual void beginTransaction() = 0;
    virtual void endTransaction() = 0;
    virtual void beginUndoGroup() = 0;
    virtual void endUndoGroup() = 0;
    virtual void notePanelEdit(const std::string& organism) = 0;
    virtual bool bypassed(const std::string& name) const = 0;

    virtual const std::string& documentPath() const = 0;
    virtual std::string documentFolder() const = 0;

    virtual bool isPlaying() const = 0;
    virtual double positionBeats() = 0;
    virtual void setPositionBeats(double beat) = 0;
    virtual void play() = 0;
    virtual double tempo() const = 0;
    virtual double groove() const = 0;
    virtual std::string grooveUnit() const = 0;

    virtual bool ensureAudio() = 0;
    virtual bool audioAlive() const = 0;
    virtual void injectLiveMidi(const MidiEvent& e) = 0;
    virtual void injectLiveMidiToNode(const std::string& node, const MidiEvent& e) = 0;
    virtual std::string exportAudioTake(const std::string& node, int take, const std::string& folder = {}) = 0;
    virtual void showVisuals(const std::string& organism, int width = 0, int height = 0) = 0;
    virtual bool canShowParameterControl() const = 0;
    virtual void showParameterControl(const std::string& organism, const std::string& param) = 0;
    virtual void noteNodeRolled(const std::string& organism) = 0;

    virtual void holdPatternSync() = 0;
    virtual void releasePatternSync() = 0;

    virtual PatternEdits& patterns() = 0;
    virtual ClipEdits& clips() = 0;
    virtual DeckEdits& decks() = 0;
    virtual FileEdits& files() = 0;
    virtual MidiEdits& midi() = 0;
    virtual OscEdits& osc() = 0;
    virtual ModEdits& mod() = 0;
    virtual AutomationEdits& automation() = 0;
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
