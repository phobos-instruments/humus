// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/ControlCord.h"
#include "gui/host/PodClip.h"

namespace hum {

class HostedPlugin;
class PluginNode;

class PatcherHost : public virtual BrickHost {
public:
    ~PatcherHost() override = default;

    virtual juce::Point<int> position(const std::string& name) const = 0;
    virtual std::string addOrganism(const std::string& className, juce::Point<int> at,
                               const std::string& podScope = {}) = 0;
    virtual void deletePod(const std::string& pod) = 0;
    virtual PodClip capturePod(const std::string& pod) const = 0;
    virtual std::string pastePod(const PodClip& clip, juce::Point<int> at,
                         const std::string& scope = {}) = 0;
    virtual void removeOrganism(const std::string& name) = 0;
    virtual void connect(const std::string& src, int outlet, const std::string& dst, int inlet) = 0;
    virtual void disconnectControl(const std::string& src, int outlet, const std::string& dst, int inlet) = 0;
    virtual std::vector<ControlCord> controlCordsInScope(const std::string& scope) = 0;
    virtual void removeConnection(const std::string& src, int outlet, const std::string& dst, int inlet) = 0;
    virtual void connectMidi(const std::string& src, int srcPort, const std::string& dst, int dstPort) = 0;
    virtual void removeMidiConnection(const std::string& src, int srcPort, const std::string& dst, int dstPort) = 0;
    virtual void connectVideo(const std::string& src, int srcPort, const std::string& dst, int dstPort) = 0;
    virtual void removeVideoConnection(const std::string& src, int srcPort, const std::string& dst, int dstPort) = 0;
    virtual bool canUndo() const = 0;
    virtual bool canRedo() const = 0;
    virtual bool undo() = 0;
    virtual bool redo() = 0;
    virtual juce::uint64 changeStamp() const = 0;
    virtual void markDirty() = 0;
    virtual void setPosition(const std::string& name, juce::Point<int> p) = 0;
    virtual std::string createPod(int ins, int outs, juce::Point<int> at,
                          const std::string& scope = {}, bool stereoPairs = false,
                          int midiIns = 0, int midiOuts = 0) = 0;
    virtual std::string makePod(const std::vector<std::string>& nodes, juce::Point<int> at,
                        const std::string& scope = {}) = 0;
    virtual bool renamePod(const std::string& pod, const std::string& newLeaf) = 0;
    virtual void ungroupPod(const std::string& pod) = 0;
    virtual void disconnectPod(const std::string& pod) = 0;
    virtual std::string insertBeforePod(const std::string& pod, const std::string& newClass) = 0;
    virtual std::string insertAfterPod(const std::string& pod, const std::string& newClass) = 0;
    virtual std::string importPatchAsPod(const std::string& path, juce::Point<int> at,
                                 const std::string& scope, std::string& error) = 0;
    virtual bool renameOrganism(const std::string& oldName, const std::string& newName) = 0;
    virtual std::string replaceOrganism(const std::string& name, const std::string& newClass) = 0;
    virtual std::string substituteOrganism(const std::string& name, const std::string& newClass) = 0;
    virtual std::string insertBefore(const std::string& name, const std::string& newClass) = 0;
    virtual std::string insertAfter(const std::string& name, const std::string& newClass) = 0;
    virtual std::string insertOnCord(const std::string& src, int outlet, const std::string& dst, int inlet,
                             const std::string& newClass, juce::Point<int> at,
                             const std::string& podScope = {}) = 0;
    virtual void swapOrganisms(const std::string& a, const std::string& b) = 0;
    virtual void disconnectOrganism(const std::string& name) = 0;
    virtual void setBypass(const std::string& name, bool on) = 0;
    virtual std::string missingClassNote(const std::string& name) const = 0;
    virtual int controlInletsOf(const std::string& name) const = 0;
    virtual int controlOutletsOf(const std::string& name) = 0;
    virtual std::string controlInletParam(const std::string& name, int inlet) const = 0;
    virtual std::string controlOutletValue(const std::string& name, int outlet) = 0;
    virtual void connectControl(const std::string& src, int outlet, const std::string& dst, int inlet) = 0;
    virtual bool isConnected(const std::string& src, int outlet, const std::string& dst, int inlet) const = 0;
    virtual bool isMidiConnected(const std::string& src, int srcPort, const std::string& dst, int dstPort) const = 0;
    virtual void setMidiCordChannel(const std::string& src, int srcPort,
                            const std::string& dst, int dstPort, int channel) = 0;
    virtual int midiCordChannel(const std::string& src, int srcPort,
                        const std::string& dst, int dstPort) const = 0;
    virtual bool isVideoConnected(const std::string& src, int srcPort, const std::string& dst, int dstPort) const = 0;
    virtual HostedPlugin* hostedPluginFor(const std::string& name) = 0;
    virtual PluginNode* pluginNodeFor(const std::string& name) = 0;
    virtual void restartPluginNode(const std::string& name) = 0;
    virtual void nodeActivity(const std::string& name, float& audioPeak, float& midiCount) = 0;
    virtual int inletsOf(const std::string& name) = 0;
    virtual int outletsOf(const std::string& name) = 0;
    virtual int midiInletsOf(const std::string& name) = 0;
    virtual int midiOutletsOf(const std::string& name) = 0;
    virtual int videoInletsOf(const std::string& name) = 0;
    virtual int videoOutletsOf(const std::string& name) = 0;
    virtual std::string printToTimeline(const std::string& node, std::string& error, int atTick = -1,
                                const std::string& preferredTarget = {}) = 0;
    virtual std::vector<std::string> noteTargets(const std::string& node) = 0;
};

}
