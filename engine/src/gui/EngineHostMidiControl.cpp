#include "gui/EngineHost.h"

#include "core/HostedPlugin.h"
#include "core/PluginNode.h"
#include "gui/AppSettings.h"
#include "gui/ControlDefaults.h"

namespace hum {

bool MidiHost::setEnabled(bool on) {
    auto& midiInputs = host_.midiInputs_;
    if (on == host_.midiEnabled_ && (!on || !midiInputs.empty())) return !midiInputs.empty();
    for (auto& in : midiInputs) in->stop();
    midiInputs.clear();
    host_.midiInputPorts_.clear();
    host_.syncSetOutput(nullptr);
    for (auto& o : host_.midiOuts_) o.reset();
    host_.midiEnabled_ = on;
    if (!on) return false;
    openConfiguredDevices();
    return !midiInputs.empty() || host_.anyMidiOutOpen();
}

void MidiHost::refreshDevices() {
    if (!host_.midiEnabled_) return;
    for (auto& in : host_.midiInputs_) in->stop();
    host_.midiInputs_.clear();
    host_.midiInputPorts_.clear();
    host_.syncSetOutput(nullptr);
    for (auto& o : host_.midiOuts_) o.reset();
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
        if (p > EngineHost::kMidiPorts) break;
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
    for (int p = 0; p < EngineHost::kMidiPorts; ++p) {
        const auto want = s.getString("midi.in." + juce::String(p + 1));
        if (want.isEmpty()) continue;
        for (const auto& dev : ins) {
            if (dev.identifier != want) continue;
            if (auto input = juce::MidiInput::openDevice(dev.identifier, &host_)) {
                input->start();
                host_.midiInputs_.push_back(std::move(input));
                host_.midiInputPorts_.push_back(p);
            }
            break;
        }
    }
    const auto outs = juce::MidiOutput::getAvailableDevices();
    for (int p = 0; p < EngineHost::kMidiPorts; ++p) {
        const auto want = s.getString("midi.out." + juce::String(p + 1));
        if (want.isEmpty()) continue;
        for (const auto& dev : outs) {
            if (dev.identifier != want) continue;
            host_.midiOuts_[p] = juce::MidiOutput::openDevice(dev.identifier);
            if (host_.midiOuts_[p]) host_.midiOuts_[p]->startBackgroundThread();
            break;
        }
    }
    host_.syncSetOutput(host_.midiOuts_[0].get());
}

bool MidiHost::enabled() const { return host_.midiEnabled_; }
const MidiControlMap& MidiHost::map() const { return host_.midiMap_; }
int  MidiHost::lastCC() const { return host_.lastCc_.load(); }
void MidiHost::clearLastCC() { host_.lastCc_.store(-1); }
int  MidiHost::sourceValue(int source) const {
    return isMidiSource(source) ? host_.ccValue_[source].load() : -1;
}
std::vector<int> MidiHost::heldNotes(int except) const {
    std::vector<int> held;
    for (int s = kNoteSourceBase; s < kMidiSourceCount; ++s)
        if (s != except && host_.ccValue_[s].load() > kHeldThreshold) held.push_back(s);
    return held;
}
void MidiHost::setModifier(int source, bool latching, bool ownAction) {
    host_.midiMap_.setModifier(source, latching, ownAction);
    host_.dirty_ = true;
}
bool MidiHost::isRecordTarget(const std::string& name) const {
    const juce::ScopedLock ml(host_.midiTargetsLock_);
    for (const auto& t : host_.midiRecordTargets_)
        if (t.node == name) return true;
    return false;
}

bool MidiHost::anyRecordTarget() const {
    const juce::ScopedLock ml(host_.midiTargetsLock_);
    return !host_.midiRecordTargets_.empty();
}

void MidiHost::syncMapFromModel() {
    host_.midiMap_.clearAll();
    for (auto& c : host_.model_.organisms)
        for (auto& s : c.midiSources) {
            const MidiSource src(s.cc, s.held);
            host_.midiMap_.set(src, c.name, s.propertyName, s.mapMin, s.mapMax);
            ControlShape sh;
            sh.smoothing = s.smoothing;
            sh.curve = s.curve;
            sh.isSwitch = s.isSwitch || paramIsSwitch(host_, c.name, s.propertyName);
            sh.inverted = s.inverted;
            sh.toggle = s.toggle;
            sh.threshold = s.threshold;
            if (!sh.isSwitch) sh.logScale = paramIsLog(host_, c.name, s.propertyName);
            if (!sh.isDefault()) host_.midiMap_.setShape(src, c.name, s.propertyName, sh);
        }
    for (const auto& m : host_.model_.midiModifiers)
        host_.midiMap_.setModifier(m.source, m.latching, m.ownAction);
}

void MidiHost::syncMapToModel() {
    std::vector<MidiControllerSource> prior;
    for (auto& c : host_.model_.organisms) {
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
    for (const auto& e : host_.midiMap_.entries()) {
        auto* cm = const_cast<OrganismModel*>(host_.model_.byName(e.organism));
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
    host_.model_.midiModifiers.clear();
    for (const auto& m : host_.midiMap_.modifiers())
        host_.model_.midiModifiers.push_back({m.source, m.latching, m.ownAction});
}

juce::String MidiHost::mapCC(const MidiSource& src, const std::string& organism,
                             const std::string& param, double min, double max, bool steal) {
    if (!isMidiSource(src.cc)) return {};
    juce::String stolenText;
    if (steal) {
        const auto stolen = host_.midiMap_.steal(src, organism, param);
        for (const auto& [sc, sp] : stolen) {
            if (stolenText.isNotEmpty()) stolenText << ", ";
            stolenText << juce::String(sc) << "/" << juce::String(sp);
        }
    }
    host_.midiMap_.set(src, organism, param, min, max);
    if (const auto* sh = host_.midiMap_.shapeOf(src, organism, param))
        if (sh->isDefault()) {
            ControlShape d;
            if (paramIsSwitch(host_, organism, param)) d.isSwitch = true;
            else d.logScale = paramIsLog(host_, organism, param);
            if (!d.isDefault()) host_.midiMap_.setShape(src, organism, param, d);
        }
    host_.dirty_ = true;
    if (!isActionTarget(host_, organism, param)) host_.automation().add(organism, param);
    host_.pokeLiveRefresh();
    return stolenText;
}

void MidiHost::setShape(const MidiSource& src, const std::string& organism,
                        const std::string& param, const ControlShape& shape) {
    host_.midiMap_.setShape(src, organism, param, shape);
    host_.dirty_ = true;
}

void MidiHost::clearCC(const MidiSource& src, const std::string& organism,
                       const std::string& param) {
    host_.midiMap_.clear(src, organism, param);
    host_.dirty_ = true;
    host_.pokeLiveRefresh();
}

void MidiHost::clearForOrganism(const std::string& organism) {
    host_.midiMap_.clearOrganism(organism);
}

void MidiHost::setReceiveMode(const std::string& name, int mode, int channel) {
    auto* cm = const_cast<OrganismModel*>(host_.model_.byName(name));
    if (!cm) return;
    if (cm->midiReceiveMode == mode && (mode != OrganismModel::kMidiChannel
                                        || cm->midiReceiveChannel == channel)) return;
    host_.pushUndo();
    cm->midiReceiveMode = mode;
    if (mode == OrganismModel::kMidiChannel && channel >= 1 && channel <= 16)
        cm->midiReceiveChannel = channel;
    host_.dirty_ = true;
    auto* hp = host_.graph_ ? dynamic_cast<PluginNode*>(host_.graph_->find(name)) : nullptr;
    if (!hp) return;
    const juce::ScopedLock ml(host_.midiTargetsLock_);
    for (auto& t : host_.midiTargets_)
        if (t.hp == hp) { t.mode = cm->midiReceiveMode; t.channel = cm->midiReceiveChannel; }
}

void MidiHost::setRecordTarget(const std::string& name, bool armed, int quantizeTicks,
                               int clip, bool thru) {
    if (!armed) host_.flushMidiRecording(true);
    const juce::ScopedLock ml(host_.midiTargetsLock_);
    auto& v = host_.midiRecordTargets_;
    const bool wasEmpty = v.empty();
    v.erase(std::remove_if(v.begin(), v.end(),
            [&](const EngineHost::MidiRecTarget& t) { return t.node == name; }), v.end());
    if (armed) {
        v.push_back({name, clip, quantizeTicks, thru});
        if (wasEmpty) { host_.midiCapture_.clear(); host_.midiRecordUndoPushed_ = false; }
    }
}

}
