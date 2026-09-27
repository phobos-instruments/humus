// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/midi/MidiControl.h"
#include "core/midi/MidiSource.h"
#include "core/net/ControlShape.h"

namespace hum {

class MidiEdits {
public:
    virtual ~MidiEdits() = default;

    virtual bool setEnabled(bool on) = 0;
    virtual bool enabled() const = 0;
    virtual void refreshDevices() = 0;
    virtual std::string mapCC(const MidiSource& src, const std::string& organism, const std::string& param,
                              double min, double max, bool steal) = 0;
    virtual std::string mapCC(int cc, const std::string& organism, const std::string& param, double min, double max,
                              bool steal) = 0;
    virtual void setShape(const MidiSource& src, const std::string& organism, const std::string& param,
                          const ControlShape& shape) = 0;
    virtual void setShape(int cc, const std::string& organism, const std::string& param, const ControlShape& shape) = 0;
    virtual void clearCC(const MidiSource& src, const std::string& organism, const std::string& param) = 0;
    virtual void clearCC(int cc, const std::string& organism, const std::string& param) = 0;
    virtual void clearForOrganism(const std::string& organism) = 0;
    virtual void setModifier(int source, bool latching, bool ownAction) = 0;
    virtual const MidiControlMap& map() const = 0;
    virtual int lastCC() const = 0;
    virtual int lastPort() const = 0;
    virtual std::string deviceForPort(int port) const = 0;
    virtual int portForDevice(const std::string& device) const = 0;
    virtual std::vector<std::string> inputDevices() const = 0;
    virtual void clearLastCC() = 0;
    virtual int sourceValue(int source) const = 0;
    virtual std::vector<int> heldNotes(int except) const = 0;
    virtual std::vector<int> recentValues(int source, double sinceMs) const = 0;
    virtual double lastSeenMs(int source) const = 0;
    virtual void setReceiveMode(const std::string& name, int mode, int channel) = 0;
    virtual void setRecordTarget(const std::string& name, bool armed, int quantizeTicks = 0, int clip = -1,
                                 bool thru = false) = 0;
    virtual void armInlet(const std::string& name, bool armed) = 0;
    virtual void disarmInstruments() = 0;
    virtual bool inletArmed(const std::string& name) const = 0;
    virtual bool anyInletArmed() const = 0;
    virtual void setTrackInput(const std::string& name, int input) = 0;
    virtual int trackInput(const std::string& name) const = 0;
    virtual unsigned inputActivity(const std::string& name) const = 0;
    virtual bool isRecordTarget(const std::string& name) const = 0;
    virtual bool anyRecordTarget() const = 0;
    virtual bool loopTakeOpen(const std::string& name) const = 0;
    virtual void clearLoop(const std::string& name) = 0;
    virtual void setRecordGrid(const std::string& name, int ticks) = 0;
};

}
