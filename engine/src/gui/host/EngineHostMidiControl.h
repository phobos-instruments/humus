// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/host/BrickHost.h"
#include "core/net/ControlShape.h"
#include "gui/host/HostCore.h"
#include "gui/host/MidiState.h"
#include "core/midi/MidiSource.h"

namespace hum {

class MidiControlMap;

class MidiHost {
public:
    static constexpr int kClipOnDemand = -2;
    MidiHost(BrickHost& host, HostCore& core, MidiState& state) : host_(host), doc_(core), audio_(core), recording_(core), state_(state) {}

    bool setEnabled(bool on);
    bool enabled() const;
    void refreshDevices();

    juce::String mapCC(const MidiSource& src, const std::string& organism,
                       const std::string& param, double min, double max, bool steal);
    juce::String mapCC(int cc, const std::string& organism, const std::string& param,
                       double min, double max, bool steal) {
        return mapCC(MidiSource(cc), organism, param, min, max, steal);
    }
    void setShape(const MidiSource& src, const std::string& organism, const std::string& param,
                  const ControlShape& shape);
    void setShape(int cc, const std::string& organism, const std::string& param,
                  const ControlShape& shape) {
        setShape(MidiSource(cc), organism, param, shape);
    }
    void clearCC(const MidiSource& src, const std::string& organism, const std::string& param);
    void clearCC(int cc, const std::string& organism, const std::string& param) {
        clearCC(MidiSource(cc), organism, param);
    }
    void clearForOrganism(const std::string& organism);
    void setModifier(int source, bool latching, bool ownAction);
    const MidiControlMap& map() const;
    int  lastCC() const;
    void clearLastCC();
    int  sourceValue(int source) const;
    std::vector<int> heldNotes(int except) const;

    void setReceiveMode(const std::string& name, int mode, int channel);

    void setRecordTarget(const std::string& name, bool armed, int quantizeTicks = 0,
                         int clip = -1, bool thru = false);
    bool isRecordTarget(const std::string& name) const;
    bool anyRecordTarget() const;

    void syncMapFromModel();
    void syncMapToModel();

private:
    void openConfiguredDevices();
    void migratePortSettings();
    BrickHost& host_;
    HostDocument& doc_;
    HostGraph& audio_;
    HostRecording& recording_;
    MidiState& state_;
};

}
