#include "gui/EngineHost.h"

#include <cmath>
#include <cstdlib>

#include "core/HostedPlugin.h"

namespace hum {

void EngineHost::sendMidiOut(const std::string& organism, const std::string& param, double value) {
    auto* cm = model_.byName(organism);
    if (!cm || cm->classRaw.rfind("MidiOut", 0) != 0) return;
    double portVal = cm->classRaw.size() > 7 ? std::atoi(cm->classRaw.c_str() + 7) : 1;
    for (auto& pr : cm->properties)
        if (pr.name == "Port") { portVal = pr.value; break; }
    const int port = juce::jlimit(0, kMidiPorts - 1, (int) portVal - 1);
    auto* out = midiOutForPort(port);
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
    for (size_t i = 0; i < midiInputs_.size(); ++i)
        if (midiInputs_[i].get() == source && i < midiInputPorts_.size()) {
            port = midiInputPorts_[i];
            break;
        }
    pushToMonitors(m, port);
    if (m.isController()) {
        const int cc = m.getControllerNumber();
        if (cc >= 0 && cc < kNoteSourceBase) {
            ccValue_[cc].store(m.getControllerValue());
            lastCc_.store(cc);
        }
        return;
    }
    if (m.isNoteOnOrOff()) {
        const int note = m.getNoteNumber();
        if (note >= 0 && note < kNoteSourceBase) {
            const int src = sourceForNote(note);
            ccValue_[src].store(m.isNoteOn() ? 127 : 0);
            if (m.isNoteOn()) lastCc_.store(src);
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

    if (!midiEnabled_) return;

    pollMidiOut();
    syncApplyChase();

    LiveControlScope live(*this);

    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const double dt = ctrlPollMs_ > 0.0 ? juce::jlimit(0.0, 0.25, (nowMs - ctrlPollMs_) / 1000.0) : 0.0;
    ctrlPollMs_ = nowMs;
    int ccVals[kMidiSourceCount];
    bool fresh[kMidiSourceCount];
    for (int cc = 0; cc < kMidiSourceCount; ++cc) {
        ccVals[cc] = ccValue_[cc].load();
        fresh[cc] = ccVals[cc] >= 0 && ccVals[cc] != ccLastApplied_[cc];
        if (fresh[cc]) ccLastApplied_[cc] = ccVals[cc];
    }
    for (const auto& u : midiMap_.tick(ccVals, fresh, dt))
        setParam(u.organism, u.param, u.value);
    for (const auto& u : osc().tickSmoothing(dt))
        setParam(u.organism, u.param, u.value);
}

}
