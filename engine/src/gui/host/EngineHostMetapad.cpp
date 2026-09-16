// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <algorithm>
#include <cmath>

#include <set>

#include "core/params/Metapad.h"
#include "core/params/MetapadPatternMorph.h"
#include "core/packs/Roles.h"

namespace hum {

namespace {
bool isNumericParam(const Parameter& p) {
    if (!p.text.empty()) return false;
    return p.type != "soundfile" && p.type != "rhythmic-unit" && p.type != "text"
           && p.name != "Pattern";
}

const AutomationLane* morphLane(const OrganismModel* cm, const char* param) {
    if (cm == nullptr) return nullptr;
    for (const auto& l : cm->automation)
        if (l.propertyName == param && !l.mute && !l.points.empty()) return &l;
    return nullptr;
}

double laneValueAt(const AutomationLane& l, double beat) {
    const auto& p = l.points;
    if (beat <= p.front().beat) return p.front().value;
    if (beat >= p.back().beat) return p.back().value;
    for (size_t i = 1; i < p.size(); ++i)
        if (p[i].beat >= beat) {
            const auto& a = p[i - 1];
            const auto& b = p[i];
            const double t = b.beat > a.beat ? (beat - a.beat) / (b.beat - a.beat) : 0.0;
            return a.value + (b.value - a.value) * t;
        }
    return p.back().value;
}

const std::string* propNameByIndex(const OrganismModel* cm, int idx) {
    if (!cm) return nullptr;
    for (auto& pr : cm->properties)
        if (pr.index == idx) return &pr.name;
    return nullptr;
}
}

void MetaEditor::apply(double x, double y) {
    LiveControlHold live(doc_);
    DerivedControlHold derived(doc_);
    auto values = interpolateMetapad(doc_.document().metapad, x, y);
    for (const auto& mv : values)
        if (auto* name = propNameByIndex(doc_.document().byName(mv.organism), mv.propertyIndex)) {
            if (mv.isRange) host_.setParamRange(mv.organism, *name, mv.value, mv.value2);
            else            host_.setParam(mv.organism, *name, mv.value);
        }
    applyMorphedPatterns(x, y);
}

void MetaEditor::applyStoredPattern(const std::string& organism, const Pattern& pattern) {
    auto* cm = doc_.mutableByName(organism);
    if (cm == nullptr || !cm->pattern.present) return;
    if (patternmorph::samePlayableContent(pattern, cm->pattern)) return;
    cm->pattern = pattern;
    patternSync_.markPatternEdited();
    doc_.bumpChangeStamp();
    patternSync_.syncPattern(organism);
}

void MetaEditor::applyMorphedPatterns(double x, double y) {
    auto& ms = doc_.document().metapad;
    std::set<std::string> names;
    for (const auto& s : ms.snapshots)
        for (const auto& sp : s.patterns) names.insert(sp.organismName);
    if (names.empty()) return;
    const auto wBySnap = snapshotWeights(ms, x, y);
    if (wBySnap.empty()) return;
    PatternSyncHold batch(host_);
    for (const auto& name : names) {
        std::vector<patternmorph::Contributor> src;
        for (const auto& s : ms.snapshots) {
            const auto it = wBySnap.find(s.index);
            if (it == wBySnap.end()) continue;
            for (const auto& sp : s.patterns)
                if (sp.organismName == name) src.push_back({&sp.pattern, it->second});
        }
        if (!src.empty()) applyStoredPattern(name, patternmorph::morph(src));
    }
}

void MetaEditor::capturePatterns(std::vector<SnapshotPattern>& out) const {
    out.clear();
    for (const auto& c : doc_.document().organisms) {
        if (!c.pattern.present) continue;
        if (classHasRole(c.classRaw, role::kAudioTrack)) continue;
        out.push_back({c.name, c.pattern});
    }
}

void MetaEditor::setTemperature(double t) {
    auto& ms = doc_.document().metapad;
    t = std::clamp(t, 0.0, kMetaTemperatureMax);
    if (ms.temperature == t) return;
    ms.temperature = t;
    doc_.flagDirty();
}

void MetaEditor::applyTarget(const std::string& param, double value) {
    if (param == kMetaSnapshotAction) {
        const bool high = value >= 0.5;
        if (high && !metaSnapArmed_) {
            addSnapshot("");
            doc_.bumpChangeStamp();
        }
        metaSnapArmed_ = high;
        return;
    }
    if (param == kMetaInterpolateParam) {
        const int mode = value >= 0.5 ? 1 : 0;
        if (doc_.document().metapad.interpolateMode == mode) return;
        doc_.document().metapad.interpolateMode = mode;
        doc_.flagDirty();
        doc_.bumpChangeStamp();
        doc_.bumpLiveControl();
        return;
    }
    if (param == kMetaTemperatureParam) {
        setTemperature(value);
        doc_.bumpLiveControl();
        return;
    }
    if (param == kMetaRecallParam) {
        const int idx = (int) std::lround(value) - 1;
        if (idx == metaLastRecall_) return;
        metaLastRecall_ = idx;
        metaRecallBeat_ = host_.positionBeats();
        if (idx >= 0) recallSnapshot(idx);
        doc_.bumpLiveControl();
        return;
    }
    if (param == kMetaXParam)      metaX_ = juce::jlimit(0.0, 1.0, value);
    else if (param == kMetaYParam) metaY_ = juce::jlimit(0.0, 1.0, value);
    else return;
    if (metaHoldApply_) return;
    apply(metaX_, metaY_);
    doc_.bumpLiveControl();
}

bool MetaEditor::hasMorphPath() const {
    const auto* cm = doc_.document().byName(nodes_.metapadNodeNameIfAny());
    return morphLane(cm, kMetaXParam) != nullptr || morphLane(cm, kMetaYParam) != nullptr
           || morphLane(cm, kMetaRecallParam) != nullptr;
}

void MetaEditor::morph(double x, double y) {
    const auto node = nodes_.metapadNodeName();
    metaHoldApply_ = true;
    host_.setParam(node, kMetaXParam, x);
    metaHoldApply_ = false;
    host_.setParam(node, kMetaYParam, y);
}

void MetaEditor::applyPathAt(double beat) {
    const auto node = nodes_.metapadNodeNameIfAny();
    const auto* cm = doc_.document().byName(node);
    const auto* lx = morphLane(cm, kMetaXParam);
    const auto* ly = morphLane(cm, kMetaYParam);
    if (lx == nullptr && ly == nullptr) return;
    auto held = [&](const char* p) {
        return host_.isPlaying() && capture_.touches().count({node, p}) > 0;
    };
    if (lx != nullptr && !held(kMetaXParam)) metaX_ = laneValueAt(*lx, beat);
    if (ly != nullptr && !held(kMetaYParam)) metaY_ = laneValueAt(*ly, beat);
    apply(metaX_, metaY_);
}

void MetaEditor::replayRecallAt(double beat) {
    const auto node = nodes_.metapadNodeNameIfAny();
    const auto* cm = doc_.document().byName(node);
    const auto* lr = morphLane(cm, kMetaRecallParam);
    if (lr == nullptr) return;
    if (host_.isPlaying() && capture_.touches().count({node, kMetaRecallParam}) > 0) return;
    if (beat < metaRecallBeat_) metaLastRecall_ = -1;
    const int idx = (int) std::lround(laneValueAt(*lr, beat)) - 1;
    if (idx == metaLastRecall_) return;
    metaLastRecall_ = idx;
    metaRecallBeat_ = beat;
    if (idx >= 0) recallSnapshot(idx);
}

int MetaEditor::addSnapshot(const std::string& name) {
    auto& ms = doc_.document().metapad;
    ms.present = true;
    int index = 0;
    for (auto& s : ms.snapshots) index = std::max(index, s.index + 1);

    DocumentSnapshot snap;
    snap.index = index;
    snap.name = name.empty() ? ("Snapshot " + std::to_string(index + 1)) : name;
    const juce::Colour c = juce::Colour::fromHSV((float) (index * 0.13) - (float) (int) (index * 0.13),
                                                 0.35f, 0.95f, 1.0f);
    snap.colour = "#" + c.toDisplayString(false).toStdString();

    const bool firstSnapshot = ms.snapshots.empty();
    for (const auto& c2 : doc_.document().organisms) {
        SnapshotOrganism sc;
        sc.organismName = c2.name;
        for (const auto& p : c2.properties)
            if (isNumericParam(p)) {
                if (p.isRange) sc.values.push_back({p.index, "range", p.rangeMin, p.rangeMax});
                else sc.values.push_back({p.index, p.type.empty() ? "double" : p.type, p.value, 0.0});
                if (firstSnapshot) ms.mask.push_back({c2.name, p.index, true});
            }
        if (!sc.values.empty()) snap.organisms.push_back(std::move(sc));
    }
    capturePatterns(snap.patterns);
    ms.snapshots.push_back(std::move(snap));
    doc_.flagDirty();
    return index;
}

void MetaEditor::storeSnapshot(int index) {
    auto& ms = doc_.document().metapad;
    for (auto& s : ms.snapshots)
        if (s.index == index) {
            s.organisms.clear();
            for (const auto& c : doc_.document().organisms) {
                SnapshotOrganism sc; sc.organismName = c.name;
                for (const auto& p : c.properties)
                    if (isNumericParam(p)) {
                        if (p.isRange) sc.values.push_back({p.index, "range", p.rangeMin, p.rangeMax});
                        else sc.values.push_back({p.index, p.type.empty() ? "double" : p.type, p.value, 0.0});
                    }
                if (!sc.values.empty()) s.organisms.push_back(std::move(sc));
            }
            capturePatterns(s.patterns);
            doc_.flagDirty();
            return;
        }
}

void MetaEditor::recallSnapshot(int index) {
    auto& ms = doc_.document().metapad;
    DerivedControlHold derived(doc_);
    for (auto& s : ms.snapshots)
        if (s.index == index) {
            for (const auto& c : s.organisms)
                for (const auto& v : c.values)
                    if (auto* name = propNameByIndex(doc_.document().byName(c.organismName), v.propertyIndex)) {
                        if (v.type == "range") host_.setParamRange(c.organismName, *name, v.value, v.value2);
                        else host_.setParam(c.organismName, *name, v.value);
                    }
            PatternSyncHold batch(host_);
            for (const auto& sp : s.patterns) applyStoredPattern(sp.organismName, sp.pattern);
            return;
        }
}

void MetaEditor::clearSnapshot(int index) {
    auto& ms = doc_.document().metapad;
    ms.snapshots.erase(std::remove_if(ms.snapshots.begin(), ms.snapshots.end(),
                                      [&](const DocumentSnapshot& s) { return s.index == index; }),
                       ms.snapshots.end());
    ms.points.erase(std::remove_if(ms.points.begin(), ms.points.end(),
                                   [&](const MetapadPoint& p) { return p.snapshotIndex == index; }),
                    ms.points.end());
    doc_.flagDirty();
}

void MetaEditor::renameSnapshot(int index, const std::string& name) {
    for (auto& s : doc_.document().metapad.snapshots)
        if (s.index == index) { s.name = name; doc_.flagDirty(); return; }
}

void MetaEditor::setSnapshotColour(int index, const std::string& hexColour) {
    for (auto& s : doc_.document().metapad.snapshots)
        if (s.index == index) { s.colour = hexColour; doc_.flagDirty(); return; }
}

void MetaEditor::placeSnapshot(int index, double x, double y) {
    doc_.document().metapad.points.push_back({index, x, y});
    doc_.document().metapad.present = true;
    doc_.flagDirty();
}

void MetaEditor::movePoint(int pointIndex, double x, double y) {
    auto& pts = doc_.document().metapad.points;
    if (pointIndex >= 0 && pointIndex < (int) pts.size()) {
        pts[(size_t) pointIndex].x = x;
        pts[(size_t) pointIndex].y = y;
        doc_.flagDirty();
    }
}

void MetaEditor::removePoint(int pointIndex) {
    auto& pts = doc_.document().metapad.points;
    if (pointIndex >= 0 && pointIndex < (int) pts.size()) {
        pts.erase(pts.begin() + pointIndex);
        doc_.flagDirty();
    }
}

void MetaEditor::setMask(const std::string& organism, int propertyIndex, bool restore) {
    for (auto& e : doc_.document().metapad.mask)
        if (e.organismName == organism && e.propertyIndex == propertyIndex) {
            e.restore = restore; doc_.flagDirty(); return;
        }
    doc_.document().metapad.mask.push_back({organism, propertyIndex, restore});
    doc_.flagDirty();
}

}
