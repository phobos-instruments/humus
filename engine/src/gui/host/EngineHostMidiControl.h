// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/host/MidiEdits.h"
#include "gui/host/BrickHost.h"
#include "core/net/ControlShape.h"
#include "gui/host/HostCore.h"
#include "gui/host/MidiState.h"
#include "core/midi/MidiSource.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"
#include "hum/caps/Midi.h"

namespace hum {

class MidiControlMap;

class MidiHost : public MidiEdits {
public:
    static constexpr int kClipOnDemand = -2;
    MidiHost(BrickHost& host, HostCore& core, MidiState& state) : host_(host), doc_(core), audio_(core), recording_(core), nodes_(core), state_(state) {}

    bool setEnabled(bool on) override;
    bool enabled() const override;
    void refreshDevices() override;

    std::string mapCC(const MidiSource& src, const std::string& organism,
                       const std::string& param, double min, double max, bool steal) override;
    std::string mapCC(int cc, const std::string& organism, const std::string& param,
                       double min, double max, bool steal) override {
        return mapCC(MidiSource(cc), organism, param, min, max, steal);
    }
    void setShape(const MidiSource& src, const std::string& organism, const std::string& param,
                  const ControlShape& shape) override;
    void setShape(int cc, const std::string& organism, const std::string& param,
                  const ControlShape& shape) override {
        setShape(MidiSource(cc), organism, param, shape);
    }
    void clearCC(const MidiSource& src, const std::string& organism, const std::string& param) override;
    void clearCC(int cc, const std::string& organism, const std::string& param) override {
        clearCC(MidiSource(cc), organism, param);
    }
    void clearForOrganism(const std::string& organism) override;
    void setModifier(int source, bool latching, bool ownAction) override;
    const MidiControlMap& map() const override;
    int  lastCC() const override;
    int  lastPort() const override;
    std::string deviceForPort(int port) const override;
    int portForDevice(const std::string& device) const override;
    std::vector<std::string> inputDevices() const override;
    void clearLastCC() override;
    int  sourceValue(int source) const override;
    std::vector<int> heldNotes(int except) const override;
    std::vector<int> recentValues(int source, double sinceMs) const override;
    double lastSeenMs(int source) const override;

    void setReceiveMode(const std::string& name, int mode, int channel) override;

    void setRecordTarget(const std::string& name, bool armed, int quantizeTicks = 0,
                         int clip = -1, bool thru = false) override;
    bool isRecordTarget(const std::string& name) const override;
    bool anyRecordTarget() const override;
    void armInlet(const std::string& name, bool armed) override;
    void disarmInstruments() override;
    bool inletArmed(const std::string& name) const override;
    bool anyInletArmed() const override;
    void setTrackInput(const std::string& name, int input) override;
    int trackInput(const std::string& name) const override;
    unsigned inputActivity(const std::string& name) const override;
    bool loopTakeOpen(const std::string& name) const override;
    void clearLoop(const std::string& name) override;
    void setRecordGrid(const std::string& name, int ticks) override;
    bool recorderSwitch(const std::string& name, const std::string& param, double value);

    void dropIdleLane(const std::string& organism, const std::string& param);

    void syncMapFromModel();
    void syncMapToModel();

private:
    struct RollTake {
        bool open = false;
        bool loopHigh = false;
        int grid = Pattern::kTicksPerBeat / 4;
    };
    NoteRecorder* recorderOf(const std::string& name);
    int rollClip(const std::string& name);
    std::vector<NoteEvent> rollNotes(const std::string& name);
    void writeRoll(const std::string& name, const std::vector<NoteEvent>& notes, int durationTicks);
    int recordQuantize(const std::string& name);
    void recordRoll(const std::string& name, bool on);
    void tapLoop(const std::string& name);
    void requantize(const std::string& name, int ticks);
    std::map<std::string, RollTake> rolls_;

    void openConfiguredDevices();
    void migratePortSettings();
    BrickHost& host_;
    HostDocument& doc_;
    HostGraph& audio_;
    HostRecording& recording_;
    HostNodes& nodes_;
    MidiState& state_;
};

}
