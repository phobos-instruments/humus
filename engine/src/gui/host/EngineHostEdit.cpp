// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include "gui/host/CordSplice.h"

#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "core/graph/CordReclaim.h"
#include "core/packs/PackManifest.h"
#include "core/plugins/PluginHost.h"
#include "core/packs/PackRegistry.h"
#include "core/graph/PodModel.h"
#include "core/packs/StripFamily.h"
#include "core/params/Randomize.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/params/ParamSchema.h"
#include "core/packs/Roles.h"
#include "gui/properties/PresetLibrary.h"
#include "hum/caps/Midi.h"
#include "hum/Registry.h"

namespace hum {

int EngineHost::reconcilePropertyTypes(PatchDocumentModel& doc) {
    int fixed = 0;
    for (auto& cm : doc.organisms) {
        if (isPluginKind(cm.kind)) continue;
        for (const auto& d : schemaFor(cm.classRaw.empty() ? cm.displayClass : cm.classRaw)) {
            if (!d.isText) continue;
            const std::string want = d.isPlainText ? "text" : "soundfile";
            for (auto& p : cm.properties) {
                if (p.name != d.name || p.type == want || p.type == "text" || p.type == "soundfile")
                    continue;
                p.type = want;
                p.userEdited = true;
                ++fixed;
            }
        }
    }
    return fixed;
}

std::string EngineHost::addOrganism(const std::string& className, juce::Point<int> at,
                                       const std::string& podScope) {
    pushUndo();
    const std::string canonical = canonicalClass(className);
    const std::string plain = (podScope.empty() ? std::string() : podScope + "/")
                            + parseClassString(className).display;
    std::string name = plain;
    for (int n = 2; model_.byName(name) != nullptr; ++n)
        name = plain + "_" + std::to_string(n);

    OrganismModel cm;
    cm.name = name;
    cm.classRaw = canonical;
    const auto classId = parseClassString(canonical);
    cm.displayClass = classId.display;
    cm.kind = classId.kind;
    int idx = 0;
    for (const auto& d : schemaFor(canonical)) {
        Parameter p;
        p.index = idx++;
        p.name = d.name;
        if (d.isText) {
            p.type = d.isPlainText ? "text" : "soundfile";
            p.text = d.text;
        } else if (d.isRange) {
            p.type = "range";
            p.isRange = true;
            p.value = d.def;
            p.rangeMin = d.def;
            p.rangeMax = d.defMax;
        } else if (!d.text.empty()) {
            p.type = "rhythmic-unit";
            p.value = d.def;
            p.text = d.text;
        } else {
            p.type = d.isBool ? "bool" : d.isEnum ? "enum" : d.isInt ? "int" : "double";
            p.value = rollDefaultValue(d, juce::Random::getSystemRandom());
        }
        cm.properties.push_back(p);
    }
    model_.organisms.push_back(std::move(cm));
    positions_[name] = at;
    requestRebuild();
    pullVoiceParams(name);
    return name;
}

std::string EngineHost::duplicateTrack(const std::string& name) {
    const auto* src = model_.byName(name);
    if (src == nullptr) return {};
    const auto cls = src->classRaw;
    const auto state = captureNodeState(name);
    const auto at = position(name) + juce::Point<int>(0, 96);

    beginTransaction();
    pushUndo();
    const auto made = addOrganism(cls, at);
    if (made.empty()) { endTransaction(); return {}; }
    if (auto* cm = mutableByName(made)) {
        cm->properties = state.props;
        cm->pattern = state.pattern;
    }
    modelSwapped_ = true;
    rebuild();
    syncPattern(made);

    const auto audio = model_.connections;
    const auto midi = model_.midiConnections;
    for (const auto& c : audio) {
        if (c.src == name) connect(made, c.srcOutlet, c.dst, c.dstInlet);
        else if (c.dst == name) connect(c.src, c.srcOutlet, made, c.dstInlet);
    }
    for (const auto& c : midi) {
        if (c.src == name) connectMidi(made, c.srcOutlet, c.dst, c.dstInlet);
        else if (c.dst == name) connectMidi(c.src, c.srcOutlet, made, c.dstInlet);
    }
    endTransaction();
    return made;
}

void EngineHost::removeOrganism(const std::string& name) {
    pushUndo();
    auto& cs = model_.organisms;
    cs.erase(std::remove_if(cs.begin(), cs.end(),
                            [&](const OrganismModel& c) { return c.name == name; }), cs.end());
    auto& cn = model_.connections;
    cn.erase(std::remove_if(cn.begin(), cn.end(),
                            [&](const ConnectionModel& c) { return c.src == name || c.dst == name; }), cn.end());
    auto& mc = model_.midiConnections;
    mc.erase(std::remove_if(mc.begin(), mc.end(),
                            [&](const ConnectionModel& c) { return c.src == name || c.dst == name; }), mc.end());
    auto& vc = model_.videoConnections;
    vc.erase(std::remove_if(vc.begin(), vc.end(),
                            [&](const ConnectionModel& c) { return c.src == name || c.dst == name; }), vc.end());
    auto& pb = model_.perfBoxes;
    pb.erase(std::remove_if(pb.begin(), pb.end(),
                            [&](const PerformanceBox& b) { return b.organism == name; }), pb.end());
    positions_.erase(name);
    requestRebuild();
}

void EngineHost::renameReferences(const std::string& oldName, const std::string& newName) {
    for (auto& c : model_.organisms) if (c.name == oldName) c.name = newName;
    for (auto& cn : model_.connections) {
        if (cn.src == oldName) cn.src = newName;
        if (cn.dst == oldName) cn.dst = newName;
    }
    for (auto& cn : model_.midiConnections) {
        if (cn.src == oldName) cn.src = newName;
        if (cn.dst == oldName) cn.dst = newName;
    }
    for (auto& cn : model_.videoConnections) {
        if (cn.src == oldName) cn.src = newName;
        if (cn.dst == oldName) cn.dst = newName;
    }
    for (auto& b : model_.perfBoxes) if (b.organism == oldName) b.organism = newName;
    for (auto& v : model_.views) if (v.organismName == oldName) v.organismName = newName;
    for (auto& av : model_.automationViews) if (av.organismName == oldName) av.organismName = newName;
    for (auto& s : model_.metapad.snapshots)
        for (auto& sc : s.organisms) if (sc.organismName == oldName) sc.organismName = newName;
    for (auto& m : model_.metapad.mask) if (m.organismName == oldName) m.organismName = newName;
    midiState_.map.renameOrganism(oldName, newName);
    osc_.renameOrganism(oldName, newName);
    mod_.renameOrganism(oldName, newName);
    auto it = positions_.find(oldName);
    if (it != positions_.end()) { positions_[newName] = it->second; positions_.erase(it); }
    if (graph_ != nullptr)
        if (auto* live = graph_->find(oldName)) live->setName(newName);
}

static std::string nameScope(const std::string& name) {
    const auto slash = name.rfind('/');
    return slash == std::string::npos ? std::string() : name.substr(0, slash + 1);
}

static std::string nameLeaf(const std::string& name) {
    return name.substr(nameScope(name).size());
}

static std::string withoutTrailingNumber(const std::string& leaf) {
    const auto us = leaf.rfind('_');
    if (us == std::string::npos || us == 0 || us + 1 >= leaf.size()) return leaf;
    if (leaf.find_first_not_of("0123456789", us + 1) != std::string::npos) return leaf;
    return leaf.substr(0, us);
}

static bool isAutoName(const std::string& name, const std::string& displayClass) {
    const std::string base = withoutTrailingNumber(nameLeaf(name));
    if (base.empty()) return false;
    return parseClassString(canonicalClass(base)).display == displayClass;
}

bool EngineHost::renameOrganism(const std::string& oldName, const std::string& newName) {
    if (oldName == newName) return true;
    if (newName.empty() || model_.byName(newName)) return false;
    pushUndo();
    renameReferences(oldName, newName);
    requestRebuild();
    return true;
}

std::string EngineHost::replaceOrganism(const std::string& nameIn, const std::string& newClassIn) {
    const std::string newClass = canonicalClass(newClassIn);
    std::string name = nameIn;
    auto* old = model_.byName(name);
    if (!old || old->displayClass == newClass) return name;
    pushUndo();
    const std::string oldDisplay = old->displayClass;

    std::map<std::string, double> kept;
    for (auto& p : old->properties) kept[p.name] = p.value;

    OrganismModel cm;
    cm.name = name;
    cm.classRaw = newClass;
    const auto classId = parseClassString(newClass);
    cm.displayClass = classId.display;
    cm.kind = classId.kind;
    int idx = 0;
    for (const auto& d : schemaFor(newClass)) {
        Parameter p;
        p.index = idx++;
        p.name = d.name;
        p.type = d.isBool ? "bool" : d.isEnum ? "enum" : d.isInt ? "int" : "double";
        auto it = kept.find(d.name);
        p.value = it != kept.end() ? it->second
                                   : rollDefaultValue(d, juce::Random::getSystemRandom());
        cm.properties.push_back(p);
    }
    for (auto& c : model_.organisms) if (c.name == name) { c = std::move(cm); break; }

    OrganismPtr probe;
    if (isPluginKind(parseClassString(newClass).kind)) probe = PluginHost::createOrganism(newClass);
    if (!probe) probe = Registry::instance().create(newClass);
    reclaimCordsForResize(model_.connections, name,
                          probe ? probe->numAudioInputs() : 0,
                          probe ? probe->numAudioOutputs() : 0);
    auto* probeMidi = dynamic_cast<MidiNode*>(probe.get());
    reclaimCordsForResize(model_.midiConnections, name,
                          probeMidi ? probeMidi->numMidiInputs() : 0,
                          probeMidi ? probeMidi->numMidiOutputs() : 0);

    const auto* wasIn = PackRegistry::instance().folderOf(canonicalClass(oldDisplay));
    const auto* nowIn = PackRegistry::instance().folderOf(newClass);
    const bool sameOrganism = wasIn != nullptr && wasIn == nowIn;

    if (!sameOrganism && isAutoName(name, oldDisplay)) {
        const std::string plain = nameScope(name) + parseClassString(newClassIn).display;
        std::string newName = plain;
        for (int n = 2; model_.byName(newName) != nullptr; ++n)
            newName = plain + "_" + std::to_string(n);
        renameReferences(name, newName);
        name = newName;
    }

    requestRebuild();
    return name;
}

void EngineHost::dropInvalidConnections() {
    auto& cn = model_.connections;
    cn.erase(std::remove_if(cn.begin(), cn.end(), [&](const ConnectionModel& c) {
        auto* s = model_.byName(c.src);
        auto* d = model_.byName(c.dst);
        if (!s || !d) return true;
        return c.srcOutlet >= classIO(s->displayClass).second
            || c.dstInlet  >= classIO(d->displayClass).first;
    }), cn.end());
}

std::string EngineHost::substituteOrganism(const std::string& name, const std::string& newClass) {
    if (!model_.byName(name)) return {};
    beginTransaction();
    const auto pos = position(name);
    const auto nn = addOrganism(newClass, pos, pods::parentOf(name));
    const auto io = classIO(newClass);

    std::vector<ConnectionModel> rerouted;
    for (auto& c : model_.connections) {
        if (c.dst == name && c.dstInlet  < io.first)  rerouted.push_back({c.src, c.srcOutlet, nn, c.dstInlet});
        if (c.src == name && c.srcOutlet < io.second) rerouted.push_back({nn, c.srcOutlet, c.dst, c.dstInlet});
    }
    std::vector<ConnectionModel> old;
    for (auto& c : model_.connections) if (c.src == name || c.dst == name) old.push_back(c);
    for (auto& c : old)      removeConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& c : rerouted) connect(c.src, c.srcOutlet, c.dst, c.dstInlet);

    const auto mio = classMidiIO(newClass);
    std::vector<ConnectionModel> midiRerouted, midiOld;
    for (auto& c : model_.midiConnections) {
        if (c.dst == name && c.dstInlet  < mio.first)  midiRerouted.push_back({c.src, c.srcOutlet, nn, c.dstInlet});
        if (c.src == name && c.srcOutlet < mio.second) midiRerouted.push_back({nn, c.srcOutlet, c.dst, c.dstInlet});
        if (c.src == name || c.dst == name) midiOld.push_back(c);
    }
    for (auto& c : midiOld)      removeMidiConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& c : midiRerouted) connectMidi(c.src, c.srcOutlet, c.dst, c.dstInlet);

    const auto vio = classVideoIO(newClass);
    std::vector<ConnectionModel> videoRerouted, videoOld;
    for (auto& c : model_.videoConnections) {
        if (c.dst == name && c.dstInlet  < vio.first)  videoRerouted.push_back({c.src, c.srcOutlet, nn, c.dstInlet});
        if (c.src == name && c.srcOutlet < vio.second) videoRerouted.push_back({nn, c.srcOutlet, c.dst, c.dstInlet});
        if (c.src == name || c.dst == name) videoOld.push_back(c);
    }
    for (auto& c : videoOld)      removeVideoConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& c : videoRerouted) connectVideo(c.src, c.srcOutlet, c.dst, c.dstInlet);

    setPosition(name, {pos.x + 24, pos.y + 70});
    endTransaction();
    return nn;
}

void EngineHost::swapOrganisms(const std::string& a, const std::string& b) {
    if (a == b || !model_.byName(a) || !model_.byName(b)) return;
    beginTransaction();
    pushUndo();
    const auto pa = position(a), pb = position(b);
    setPosition(a, pb);
    setPosition(b, pa);
    for (auto& c : model_.connections) {
        if (c.src == a) c.src = b; else if (c.src == b) c.src = a;
        if (c.dst == a) c.dst = b; else if (c.dst == b) c.dst = a;
    }
    dropInvalidConnections();
    requestRebuild();
    endTransaction();
}

}
