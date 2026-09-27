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
int  MidiHost::lastPort() const { return state_.lastPort.load(); }
void MidiHost::clearLastCC() { state_.lastCc.store(-1); }
int  MidiHost::sourceValue(int source) const {
    return isMidiSource(source) ? state_.ccValue[kAnyMidiPortRow][source].load() : -1;
}
std::vector<int> MidiHost::heldNotes(int except) const {
    std::vector<int> held;
    for (int s = kNoteSourceBase; s < kNoteSourceEnd; ++s)
        if (s != except && state_.ccValue[kAnyMidiPortRow][s].load() > kHeldThreshold)
            held.push_back(s);
    return held;
}
std::vector<int> MidiHost::recentValues(int source, double sinceMs) const {
    std::vector<int> out;
    for (const auto& e : state_.feed.trail())
        if (e.source == source && e.atMs >= sinceMs) out.push_back(e.value);
    return out;
}
double MidiHost::lastSeenMs(int source) const {
    const auto& trail = state_.feed.trail();
    for (auto it = trail.rbegin(); it != trail.rend(); ++it)
        if (it->source == source) return it->atMs;
    return -1.0;
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

std::string MidiHost::deviceForPort(int port) const {
    if (port < 0 || port >= kMidiPorts) return {};
    for (size_t i = 0; i < state_.inputs.size() && i < state_.inputPorts.size(); ++i)
        if (state_.inputPorts[i] == port && state_.inputs[i])
            return state_.inputs[i]->getName().toStdString();
    return {};
}

int MidiHost::portForDevice(const std::string& device) const {
    if (device.empty()) return kAnyMidiPort;
    for (size_t i = 0; i < state_.inputs.size() && i < state_.inputPorts.size(); ++i)
        if (state_.inputs[i] && state_.inputs[i]->getName().toStdString() == device)
            return state_.inputPorts[i];
    return kMidiPorts;
}

std::vector<std::string> MidiHost::inputDevices() const {
    std::vector<std::string> out;
    for (const auto& in : state_.inputs)
        if (in) out.push_back(in->getName().toStdString());
    return out;
}

void MidiHost::syncMapFromModel() {
    state_.map.clearAll();
    for (auto& c : doc_.document().organisms)
        for (auto& s : c.midiSources) {
            const MidiSource src(s.cc, s.held, portForDevice(s.device), s.channel);
            state_.map.set(src, c.name, s.propertyName, s.mapMin, s.mapMax);
            ControlShape sh = s.shape;
            if (!sh.isButton() && paramIsSwitch(host_, c.name, s.propertyName) && !sh.isEncoder())
                sh.type = ControlType::Button;
            sh.logScale = sh.isFader() && paramIsLog(host_, c.name, s.propertyName);
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
            if (p.propertyName == key && p.cc == src.cc && p.held == src.held
                && midiChannelSlot(p.channel) == src.channel
                && portForDevice(p.device) == midiPortRow(src.port))
                return &p;
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
        s.channel = e.channel;
        if (e.port < 0) s.device.clear();
        else if (const auto named = deviceForPort(e.port); !named.empty()) s.device = named;
        s.mapMin = e.min;
        s.mapMax = e.max;
        s.shape = e.shape;
        s.shape.logScale = false;
        cm->midiSources.push_back(std::move(s));
    }
    doc_.document().midiModifiers.clear();
    for (const auto& m : state_.map.modifiers())
        doc_.document().midiModifiers.push_back({m.source, m.latching, m.ownAction});
}

std::string MidiHost::mapCC(const MidiSource& src, const std::string& organism,
                            const std::string& param, double min, double max, bool steal) {
    if (!isMidiSource(src.cc)) return {};
    std::string stolenText;
    if (steal)
        for (const auto& [sc, sp] : state_.map.steal(src, organism, param))
            stolenText += (stolenText.empty() ? "" : ", ") + sc + "/" + sp;
    state_.map.set(src, organism, param, min, max);
    if (const auto* sh = state_.map.shapeOf(src, organism, param))
        if (sh->isDefault()) {
            ControlShape d;
            if (paramIsSwitch(host_, organism, param)) d.type = ControlType::Button;
            else d.logScale = paramIsLog(host_, organism, param);
            if (!d.isDefault()) state_.map.setShape(src, organism, param, d);
        }
    doc_.flagDirty();
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
    dropIdleLane(organism, param);
    doc_.flagDirty();
    doc_.pokeLiveRefresh();
}

void MidiHost::dropIdleLane(const std::string& organism, const std::string& param) {
    for (const auto& e : state_.map.entries())
        if (e.organism == organism && e.param == param) return;
    for (const auto& e : host_.osc().map().entries())
        if (e.organism == organism && e.param == param) return;
    for (const auto& e : host_.mod().map().entries())
        if (e.organism == organism && e.param == param) return;
    const auto* cm = doc_.document().byName(organism);
    if (cm == nullptr) return;
    for (const auto& l : cm->automation)
        if (l.propertyName == param) {
            if (l.points.size() > 1) return;
            host_.automation().remove(organism, param);
            return;
        }
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

void MidiHost::armInlet(const std::string& name, bool armed) {
    bool existing = false;
    {
        const juce::ScopedLock ml(state_.targetsLock);
        for (auto& t : state_.recordTargets)
            if (t.node == name) {
                existing = true;
                t.inlet = armed;
            }
    }
    if (!existing && armed) {
        setRecordTarget(name, true, 0, kClipOnDemand, false);
        const juce::ScopedLock ml(state_.targetsLock);
        for (auto& t : state_.recordTargets)
            if (t.node == name) t.inlet = true;
    } else if (existing && !armed) {
        setRecordTarget(name, false);
    }
    if (auto* g = audio_.graph(); g != nullptr && !armed) {
        const juce::ScopedLock sl(audio_.graphLock());
        g->setMidiInletTap(g->indexOf(name), false);
    }
    nodes_.refreshArmedInputs();
}

void MidiHost::disarmInstruments() {
    const juce::ScopedLock ml(state_.targetsLock);
    auto& v = state_.recordTargets;
    v.erase(std::remove_if(v.begin(), v.end(),
                           [](const MidiState::RecordTarget& t) { return !t.inlet && t.fromInput; }),
            v.end());
    for (auto& t : v)
        if (t.inlet) {
            t.clip = kClipOnDemand;
            t.grow = false;
            t.takeColour = kNoColour;
        }
}

void MidiHost::setTrackInput(const std::string& name, int input) {
    auto* cm = doc_.document().byName(name);
    if (cm == nullptr || cm->trackInput == input) return;
    host_.pushUndo();
    cm->trackInput = input;
    doc_.flagDirty();
    nodes_.refreshArmedInputs();
}

int MidiHost::trackInput(const std::string& name) const {
    const auto* cm = doc_.document().byName(name);
    return cm != nullptr ? cm->trackInput : OrganismModel::kTrackInputAuto;
}

unsigned MidiHost::inputActivity(const std::string& name) const {
    unsigned hits = 0;
    {
        const juce::ScopedLock ml(state_.targetsLock);
        if (const auto it = state_.keyboardHits.find(name); it != state_.keyboardHits.end()) hits = it->second;
    }
    if (auto* g = audio_.graph()) hits += g->nodeMidiInletCount(g->indexOf(name));
    return hits;
}

bool MidiHost::anyInletArmed() const {
    const juce::ScopedLock ml(state_.targetsLock);
    for (const auto& t : state_.recordTargets)
        if (t.inlet) return true;
    return false;
}

bool MidiHost::inletArmed(const std::string& name) const {
    const juce::ScopedLock ml(state_.targetsLock);
    for (const auto& t : state_.recordTargets)
        if (t.node == name && t.inlet) return true;
    return false;
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
