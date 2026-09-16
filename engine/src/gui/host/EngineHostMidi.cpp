// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <cmath>
#include <cstdlib>

#include "core/plugins/HostedPlugin.h"
#include "core/packs/Roles.h"

#include "hum/dsp/DspMath.h"

namespace hum {

void EngineHost::sendMidiOut(const std::string& organism, const std::string& param, double value) {
    auto* cm = model_.byName(organism);
    if (!cm || !classHasRole(cm->classRaw, role::kMidiOut)) return;
    const auto numbered = cm->classRaw.find_last_not_of("0123456789") + 1;
    double portVal = numbered < cm->classRaw.size() ? std::atoi(cm->classRaw.c_str() + numbered) : 1;
    for (auto& pr : cm->properties)
        if (pr.name == "Port") { portVal = pr.value; break; }
    const int port = juce::jlimit(0, kMidiPorts - 1, (int) portVal - 1);
    auto* out = midiState_.outForPort(port);
    if (out == nullptr) return;
    auto pv = [&](const char* n, double def) {
        for (auto& pr : cm->properties) if (pr.name == n) return pr.value;
        return def;
    };
    for (const auto& msg : midiOutMessages(param, (int) pv("Channel", 1), (int) pv("Controller", 1),
                                           (int) pv("Value", 0), (int) pv("Note", 60),
                                           (int) pv("Velocity", 100), pv("Gate", 0)))
        out->sendMessageNow(juce::MidiMessage(msg.status, msg.data1, msg.data2));
}

void EngineHost::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& m) {
    if (syncHandleMidi(m)) return;
    if (m.isMidiClock() || m.isActiveSense()) return;
    int port = 0;
    for (size_t i = 0; i < midiState_.inputs.size(); ++i)
        if (midiState_.inputs[i].get() == source && i < midiState_.inputPorts.size()) {
            port = midiState_.inputPorts[i];
            break;
        }
    pushToMonitors(m, port);
    if (m.isController()) {
        const int cc = m.getControllerNumber();
        if (cc >= 0 && cc < kNoteSourceBase) {
            midiState_.ccValue[cc].store(m.getControllerValue());
            midiState_.lastCc.store(cc);
        }
        return;
    }
    if (m.isNoteOnOrOff()) {
        const int note = m.getNoteNumber();
        if (note >= 0 && note < kNoteSourceBase) {
            const int src = sourceForNote(note);
            midiState_.ccValue[src].store(m.isNoteOn() ? kMidiMax : 0);
            if (m.isNoteOn()) midiState_.lastCc.store(src);
        }
    }
    routeLiveMidi(m, port);
}

void EngineHost::pollMidiControl() {
    flushMidiRecording();
    extendLatchPasses();

    if (playing_ && !capturing_ && metapad().hasMorphPath()) {
        const double b = positionBeats();
        if (std::abs(b - lastMorphBeat_) > 1e-4) {
            lastMorphBeat_ = b;
            metapad().applyPathAt(b);
            metapad().replayRecallAt(b);
        }
    }
    if (playing_) trimPerfRings();

    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const double mdt = modPollMs_ > 0.0
                               ? juce::jlimit(0.0, 0.25, (nowMs - modPollMs_) / 1000.0) : 0.0;
        modPollMs_ = nowMs;
        if (!mod_.map().empty()) {
            LiveControlScope live(*this);
            for (const auto& u : mod_.tick(mdt))
                setParam(u.organism, u.param, u.value);
        }
    }

    linkApplyUi();

    if (!midiState_.enabled) return;

    pollMidiOut();
    syncApplyChase();

    LiveControlScope live(*this);

    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const double dt = ctrlPollMs_ > 0.0 ? juce::jlimit(0.0, 0.25, (nowMs - ctrlPollMs_) / 1000.0) : 0.0;
    ctrlPollMs_ = nowMs;
    int ccVals[kMidiSourceCount];
    bool fresh[kMidiSourceCount];
    for (int cc = 0; cc < kMidiSourceCount; ++cc) {
        ccVals[cc] = midiState_.ccValue[cc].load();
        fresh[cc] = ccVals[cc] >= 0 && ccVals[cc] != midiState_.ccLastApplied[cc];
        if (fresh[cc]) midiState_.ccLastApplied[cc] = ccVals[cc];
    }
    for (const auto& u : midiState_.map.tick(ccVals, fresh, dt))
        setParam(u.organism, u.param, u.value);
    for (const auto& u : osc().tickSmoothing(dt))
        setParam(u.organism, u.param, u.value);
}

}
