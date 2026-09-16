// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>

#include "gui/host/BrickHost.h"

namespace juce { template <typename> class Point; }

namespace hum {

class AssistantHost : public virtual BrickHost {
public:
    ~AssistantHost() override = default;

    virtual juce::Point<int> position(const std::string& name) const = 0;
    virtual std::string addOrganism(const std::string& className, juce::Point<int> at,
                               const std::string& podScope = {}) = 0;
    virtual void removeOrganism(const std::string& name) = 0;
    virtual bool renameOrganism(const std::string& oldName, const std::string& newName) = 0;
    virtual std::string replaceOrganism(const std::string& name, const std::string& newClass) = 0;
    virtual void connect(const std::string& src, int outlet, const std::string& dst, int inlet) = 0;
    virtual void removeConnection(const std::string& src, int outlet, const std::string& dst, int inlet) = 0;
    virtual void connectMidi(const std::string& src, int srcPort, const std::string& dst, int dstPort) = 0;
    virtual void removeMidiConnection(const std::string& src, int srcPort, const std::string& dst, int dstPort) = 0;
    virtual void nodeActivity(const std::string& name, float& audioPeak, float& midiCount) = 0;
    virtual bool armNodeCapture(int samples) = 0;
    virtual void disarmNodeCapture() = 0;
    virtual int copyNodeCapture(const std::string& name, std::vector<float>& out) = 0;
    virtual double sampleRate() const = 0;
    virtual void playFromStart() = 0;
    virtual void stop() = 0;
    virtual void performTempo(double bpm) = 0;
    virtual int outletsOf(const std::string& name) = 0;
};

}
