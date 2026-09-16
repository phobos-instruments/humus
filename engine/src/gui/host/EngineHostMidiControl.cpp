// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHostMidiControl.h"
#include "core/graph/AudioGraph.h"
#include "gui/host/EngineHostAutomation.h"

#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginNode.h"
#include "gui/app/AppSettings.h"
#include "gui/editor/ControlDefaults.h"

namespace hum {

bool MidiHost::setEnabled(bool on) {
    auto& midiInputs = state_.inputs;
    if (on == state_.enabled && (!on || !midiInputs.empty())) return !midiInputs.empty();
    for (auto& in : midiInputs) in->stop();
    midiInputs.clear();
    state_.inputPorts.clear();
    audio_.setSyncOutput(nullptr);
    for (auto& o : state_.outs) o.reset();
    state_.enabled = on;
    if (!on) return false;
    openConfiguredDevices();
    return !midiInputs.empty() || state_.anyOutOpen();
}

void MidiHost::refreshDevices() {
    if (!state_.enabled) return;
    for (auto& in : state_.inputs) in->stop();
    state_.inputs.clear();
    state_.inputPorts.clear();
    audio_.setSyncOutput(nullptr);
    for (auto& o : state_.outs) o.reset();
    openConfiguredDevices();
}

void MidiHost::migratePortSettings() {
    auto& s = AppSettings::instance();
    if (s.getInt("midi.ports", 0) >= 1) return;
    s.set("midi.ports", 1);
    juce::StringArray disabled;
    disabled.addLines(s.getString("midi.disabledInputs"));
    int p = 1;
    for (const auto& dev : juce::MidiInput::getAvailableDevices()) {
        if (disabled.contains(dev.identifier)) continue;
        if (p > MidiState::kPorts) break;
        s.set("midi.in." + juce::String(p), dev.identifier);
        s.set("midi.in." + juce::String(p) + ".name", dev.name);
        ++p;
    }
    auto out = s.getString("midi.output");
    juce::String outName;
    if (out.isEmpty()) {
        const auto def = juce::MidiOutput::getDefaultDevice();
        out = def.identifier;
        outName = def.name;
    } else {
        for (const auto& d : juce::MidiOutput::getAvailableDevices())
            if (d.identifier == out) { outName = d.name; break; }
    }
    if (out.isNotEmpty()) {
        s.set("midi.out.1", out);
        s.set("midi.out.1.name", outName);
    }
}

void MidiHost::openConfiguredDevices() {
    migratePortSettings();
    auto& s = AppSettings::instance();
    const auto ins = juce::MidiInput::getAvailableDevices();
    for (int p = 0; p < MidiState::kPorts; ++p) {
        const auto want = s.getString("midi.in." + juce::String(p + 1));
        if (want.isEmpty()) continue;
        for (const auto& dev : ins) {
            if (dev.identifier != want) continue;
            if (auto input = juce::MidiInput::openDevice(dev.identifier, &audio_.midiInputSink())) {
                input->start();
                state_.inputs.push_back(std::move(input));
                state_.inputPorts.push_back(p);
            }
            break;
        }
    }
    const auto outs = juce::MidiOutput::getAvailableDevices();
    for (int p = 0; p < MidiState::kPorts; ++p) {
        const auto want = s.getString("midi.out." + juce::String(p + 1));
        if (want.isEmpty()) continue;
        for (const auto& dev : outs) {
            if (dev.identifier != want) continue;
            state_.outs[p] = juce::MidiOutput::openDevice(dev.identifier);
            if (state_.outs[p]) state_.outs[p]->startBackgroundThread();
            break;
        }
    }
    audio_.setSyncOutput(state_.outs[0].get());
}

bool MidiHost::enabled() const { return state_.enabled; }
const MidiControlMap& MidiHost::map() const { return state_.map; }
int  MidiHost::lastCC() const { return state_.lastCc.load(); }
void MidiHost::clearLastCC() { state_.lastCc.store(-1); }
int  MidiHost::sourceValue(int source) const {
    return isMidiSource(source) ? state_.ccValue[source].load() : -1;
}
std::vector<int> MidiHost::heldNotes(int except) const {
    std::vector<int> held;
    for (int s = kNoteSourceBase; s < kMidiSourceCount; ++s)
        if (s != except && state_.ccValue[s].load() > kHeldThreshold) held.push_back(s);
    return held;
}
void MidiHost::setModifier(int source, bool latching, bool ownAction) {
    state_.map.setModifier(source, latching, ownAction);
    doc_.flagDirty();
}
bool MidiHost::isRecordTarget(const std::string& name) const {
    const juce::ScopedLock ml(state_.targetsLock);
    for (const auto& t : state_.recordTargets)
        if (t.node == name) return true;
    return false;
}

bool MidiHost::anyRecordTarget() const {
    const juce::ScopedLock ml(state_.targetsLock);
    return !state_.recordTargets.empty();
}

void MidiHost::syncMapFromModel() {
    state_.map.clearAll();
    for (auto& c : doc_.document().organisms)
        for (auto& s : c.midiSources) {
            const MidiSource src(s.cc, s.held);
            state_.map.set(src, c.name, s.propertyName, s.mapMin, s.mapMax);
            ControlShape sh;
            sh.smoothing = s.smoothing;
            sh.curve = s.curve;
            sh.isSwitch = s.isSwitch || paramIsSwitch(host_, c.name, s.propertyName);
            sh.inverted = s.inverted;
            sh.toggle = s.toggle;
            sh.threshold = s.threshold;
            if (!sh.isSwitch) sh.logScale = paramIsLog(host_, c.name, s.propertyName);
            if (!sh.isDefault()) state_.map.setShape(src, c.name, s.propertyName, sh);
        }
    for (const auto& m : doc_.document().midiModifiers)
        state_.map.setModifier(m.source, m.latching, m.ownAction);
}

void MidiHost::syncMapToModel() {
    std::vector<MidiControllerSource> prior;
    for (auto& c : doc_.document().organisms) {
        for (auto& s : c.midiSources) { s.propertyName = c.name + "\t" + s.propertyName; prior.push_back(s); }
        c.midiSources.clear();
    }
    auto findPrior = [&](const std::string& name, const std::string& param,
                         const MidiSource& src) -> const MidiControllerSource* {
        const std::string key = name + "\t" + param;
        for (auto& p : prior)
            if (p.propertyName == key && p.cc == src.cc && p.held == src.held) return &p;
        return nullptr;
    };
    for (const auto& e : state_.map.entries()) {
        auto* cm = doc_.document().byName(e.organism);
        if (!cm) continue;
        MidiControllerSource s;
        if (auto* old = findPrior(e.organism, e.param, e.source())) {
            s = *old;
        }
        s.propertyName = e.param;
        s.propertyIndex = -1;
        for (auto& pr : cm->properties)
            if (pr.name == e.param) { s.propertyIndex = pr.index; break; }
        s.cc = e.cc;
        s.held = e.held;
        s.mapMin = e.min;
        s.mapMax = e.max;
        s.smoothing = e.shape.smoothing;
        s.curve = e.shape.curve;
        s.isSwitch = e.shape.isSwitch;
        s.inverted = e.shape.inverted;
        s.toggle = e.shape.toggle;
        s.threshold = e.shape.threshold;
        cm->midiSources.push_back(std::move(s));
    }
    doc_.document().midiModifiers.clear();
    for (const auto& m : state_.map.modifiers())
        doc_.document().midiModifiers.push_back({m.source, m.latching, m.ownAction});
}

juce::String MidiHost::mapCC(const MidiSource& src, const std::string& organism,
                             const std::string& param, double min, double max, bool steal) {
    if (!isMidiSource(src.cc)) return {};
    juce::String stolenText;
    if (steal) {
        const auto stolen = state_.map.steal(src, organism, param);
        for (const auto& [sc, sp] : stolen) {
            if (stolenText.isNotEmpty()) stolenText << ", ";
            stolenText << juce::String(sc) << "/" << juce::String(sp);
        }
    }
    state_.map.set(src, organism, param, min, max);
    if (const auto* sh = state_.map.shapeOf(src, organism, param))
        if (sh->isDefault()) {
            ControlShape d;
            if (paramIsSwitch(host_, organism, param)) d.isSwitch = true;
            else d.logScale = paramIsLog(host_, organism, param);
            if (!d.isDefault()) state_.map.setShape(src, organism, param, d);
        }
    doc_.flagDirty();
    if (!isActionTarget(host_, organism, param)) host_.automation().add(organism, param);
    doc_.pokeLiveRefresh();
    return stolenText;
}

void MidiHost::setShape(const MidiSource& src, const std::string& organism,
                        const std::string& param, const ControlShape& shape) {
    state_.map.setShape(src, organism, param, shape);
    doc_.flagDirty();
}

void MidiHost::clearCC(const MidiSource& src, const std::string& organism,
                       const std::string& param) {
    state_.map.clear(src, organism, param);
    doc_.flagDirty();
    doc_.pokeLiveRefresh();
}

void MidiHost::clearForOrganism(const std::string& organism) {
    state_.map.clearOrganism(organism);
}

void MidiHost::setReceiveMode(const std::string& name, int mode, int channel) {
    auto* cm = doc_.document().byName(name);
    if (!cm) return;
    if (cm->midiReceiveMode == mode && (mode != OrganismModel::kMidiChannel
                                        || cm->midiReceiveChannel == channel)) return;
    host_.pushUndo();
    cm->midiReceiveMode = mode;
    if (mode == OrganismModel::kMidiChannel && channel >= 1 && channel <= 16)
        cm->midiReceiveChannel = channel;
    doc_.flagDirty();
    auto* hp = audio_.graph() ? dynamic_cast<PluginNode*>(audio_.graph()->find(name)) : nullptr;
    if (!hp) return;
    const juce::ScopedLock ml(state_.targetsLock);
    for (auto& t : state_.targets)
        if (t.hp == hp) { t.mode = cm->midiReceiveMode; t.channel = cm->midiReceiveChannel; }
}

void MidiHost::setRecordTarget(const std::string& name, bool armed, int quantizeTicks,
                               int clip, bool thru) {
    if (!armed) recording_.flushMidiRecording(true);
    const juce::ScopedLock ml(state_.targetsLock);
    auto& v = state_.recordTargets;
    const bool wasEmpty = v.empty();
    v.erase(std::remove_if(v.begin(), v.end(),
            [&](const MidiState::RecordTarget& t) { return t.node == name; }), v.end());
    if (armed) {
        v.push_back({name, clip, quantizeTicks, thru});
        if (wasEmpty) { state_.capture.clear(); state_.recordUndoPushed = false; }
    }
}

}
