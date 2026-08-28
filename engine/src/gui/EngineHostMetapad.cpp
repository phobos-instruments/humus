#include "gui/EngineHost.h"

#include <algorithm>
#include <cmath>

#include <set>

#include "core/Metapad.h"
#include "core/MetapadPatternMorph.h"

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
    EngineHost::LiveControlScope live(host_);
    EngineHost::DerivedControlScope derived(host_);
    auto values = interpolateMetapad(host_.model_.metapad, x, y);
    for (const auto& mv : values)
        if (auto* name = propNameByIndex(host_.model_.byName(mv.organism), mv.propertyIndex)) {
            if (mv.isRange) host_.setParamRange(mv.organism, *name, mv.value, mv.value2);
            else            host_.setParam(mv.organism, *name, mv.value);
        }
    applyMorphedPatterns(x, y);
}

void MetaEditor::applyStoredPattern(const std::string& organism, const Pattern& pattern) {
    auto* cm = host_.mutableByName(organism);
    if (cm == nullptr || !cm->pattern.present) return;
    if (patternmorph::samePlayableContent(pattern, cm->pattern)) return;
    cm->pattern = pattern;
    host_.markPatternEdited();
    ++host_.changeStamp_;
    host_.syncPattern(organism);
}

void MetaEditor::applyMorphedPatterns(double x, double y) {
    auto& ms = host_.model_.metapad;
    std::set<std::string> names;
    for (const auto& s : ms.snapshots)
        for (const auto& sp : s.patterns) names.insert(sp.organismName);
    if (names.empty()) return;
    const auto wBySnap = snapshotWeights(ms, x, y);
    if (wBySnap.empty()) return;
    EngineHost::PatternSyncBatch batch(host_);
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
    for (const auto& c : host_.model_.organisms) {
        if (!c.pattern.present) continue;
        if (c.displayClass == "AudioTrack") continue;
        out.push_back({c.name, c.pattern});
    }
}

void MetaEditor::setTemperature(double t) {
    auto& ms = host_.model_.metapad;
    t = std::clamp(t, 0.0, kMetaTemperatureMax);
    if (ms.temperature == t) return;
    ms.temperature = t;
    host_.dirty_ = true;
}

std::string EngineHost::metapadNodeName() {
    for (auto& cm : model_.organisms)
        if (isMetapadPseudo(cm.displayClass)) return cm.name;
    OrganismModel cm;
    cm.name = "Metapad";
    while (model_.byName(cm.name) != nullptr) cm.name += "_";
    cm.classRaw = "MetasurfacePseudoSP";
    cm.displayClass = "MetasurfacePseudoSP";
    model_.organisms.push_back(std::move(cm));
    return model_.organisms.back().name;
}

void EngineHost::applyMetapadTarget(const std::string& param, double value) {
    if (param == kMetaSnapshotAction) {
        const bool high = value >= 0.5;
        if (high && !metaSnapArmed_) {
            metapad().addSnapshot("");
            ++changeStamp_;
        }
        metaSnapArmed_ = high;
        return;
    }
    if (param == kMetaInterpolateParam) {
        const int mode = value >= 0.5 ? 1 : 0;
        if (model_.metapad.interpolateMode == mode) return;
        model_.metapad.interpolateMode = mode;
        dirty_ = true;
        ++changeStamp_;
        ++liveControlGen_;
        return;
    }
    if (param == kMetaTemperatureParam) {
        metapad().setTemperature(value);
        ++liveControlGen_;
        return;
    }
    if (param == kMetaRecallParam) {
        const int idx = (int) std::lround(value) - 1;
        if (idx == metaLastRecall_) return;
        metaLastRecall_ = idx;
        metaRecallBeat_ = positionBeats();
        if (idx >= 0) metapad().recallSnapshot(idx);
        ++liveControlGen_;
        return;
    }
    if (param == kMetaXParam)      metaX_ = juce::jlimit(0.0, 1.0, value);
    else if (param == kMetaYParam) metaY_ = juce::jlimit(0.0, 1.0, value);
    else return;
    if (metaHoldApply_) return;
    metapad().apply(metaX_, metaY_);
    ++liveControlGen_;
}

bool MetaEditor::hasMorphPath() const {
    const auto* cm = host_.model_.byName(host_.metapadNodeNameIfAny());
    return morphLane(cm, kMetaXParam) != nullptr || morphLane(cm, kMetaYParam) != nullptr
           || morphLane(cm, kMetaRecallParam) != nullptr;
}

void MetaEditor::morph(double x, double y) {
    const auto node = host_.metapadNodeName();
    host_.metaHoldApply_ = true;
    host_.setParam(node, kMetaXParam, x);
    host_.metaHoldApply_ = false;
    host_.setParam(node, kMetaYParam, y);
}

void MetaEditor::applyPathAt(double beat) {
    const auto node = host_.metapadNodeNameIfAny();
    const auto* cm = host_.model_.byName(node);
    const auto* lx = morphLane(cm, kMetaXParam);
    const auto* ly = morphLane(cm, kMetaYParam);
    if (lx == nullptr && ly == nullptr) return;
    auto held = [&](const char* p) {
        return host_.playing_ && host_.touched_.count({node, p}) > 0;
    };
    if (lx != nullptr && !held(kMetaXParam)) host_.metaX_ = laneValueAt(*lx, beat);
    if (ly != nullptr && !held(kMetaYParam)) host_.metaY_ = laneValueAt(*ly, beat);
    apply(host_.metaX_, host_.metaY_);
}

void MetaEditor::replayRecallAt(double beat) {
    const auto node = host_.metapadNodeNameIfAny();
    const auto* cm = host_.model_.byName(node);
    const auto* lr = morphLane(cm, kMetaRecallParam);
    if (lr == nullptr) return;
    if (host_.playing_ && host_.touched_.count({node, kMetaRecallParam}) > 0) return;
    if (beat < host_.metaRecallBeat_) host_.metaLastRecall_ = -1;
    const int idx = (int) std::lround(laneValueAt(*lr, beat)) - 1;
    if (idx == host_.metaLastRecall_) return;
    host_.metaLastRecall_ = idx;
    host_.metaRecallBeat_ = beat;
    if (idx >= 0) recallSnapshot(idx);
}

int MetaEditor::addSnapshot(const std::string& name) {
    auto& ms = host_.model_.metapad;
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
    for (const auto& c2 : host_.model_.organisms) {
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
    host_.dirty_ = true;
    return index;
}

void MetaEditor::storeSnapshot(int index) {
    auto& ms = host_.model_.metapad;
    for (auto& s : ms.snapshots)
        if (s.index == index) {
            s.organisms.clear();
            for (const auto& c : host_.model_.organisms) {
                SnapshotOrganism sc; sc.organismName = c.name;
                for (const auto& p : c.properties)
                    if (isNumericParam(p)) {
                        if (p.isRange) sc.values.push_back({p.index, "range", p.rangeMin, p.rangeMax});
                        else sc.values.push_back({p.index, p.type.empty() ? "double" : p.type, p.value, 0.0});
                    }
                if (!sc.values.empty()) s.organisms.push_back(std::move(sc));
            }
            capturePatterns(s.patterns);
            host_.dirty_ = true;
            return;
        }
}

void MetaEditor::recallSnapshot(int index) {
    auto& ms = host_.model_.metapad;
    EngineHost::DerivedControlScope derived(host_);
    for (auto& s : ms.snapshots)
        if (s.index == index) {
            for (const auto& c : s.organisms)
                for (const auto& v : c.values)
                    if (auto* name = propNameByIndex(host_.model_.byName(c.organismName), v.propertyIndex)) {
                        if (v.type == "range") host_.setParamRange(c.organismName, *name, v.value, v.value2);
                        else host_.setParam(c.organismName, *name, v.value);
                    }
            EngineHost::PatternSyncBatch batch(host_);
            for (const auto& sp : s.patterns) applyStoredPattern(sp.organismName, sp.pattern);
            return;
        }
}

void MetaEditor::clearSnapshot(int index) {
    auto& ms = host_.model_.metapad;
    ms.snapshots.erase(std::remove_if(ms.snapshots.begin(), ms.snapshots.end(),
                                      [&](const DocumentSnapshot& s) { return s.index == index; }),
                       ms.snapshots.end());
    ms.points.erase(std::remove_if(ms.points.begin(), ms.points.end(),
                                   [&](const MetapadPoint& p) { return p.snapshotIndex == index; }),
                    ms.points.end());
    host_.dirty_ = true;
}

void MetaEditor::renameSnapshot(int index, const std::string& name) {
    for (auto& s : host_.model_.metapad.snapshots)
        if (s.index == index) { s.name = name; host_.dirty_ = true; return; }
}

void MetaEditor::setSnapshotColour(int index, const std::string& hexColour) {
    for (auto& s : host_.model_.metapad.snapshots)
        if (s.index == index) { s.colour = hexColour; host_.dirty_ = true; return; }
}

void MetaEditor::placeSnapshot(int index, double x, double y) {
    host_.model_.metapad.points.push_back({index, x, y});
    host_.model_.metapad.present = true;
    host_.dirty_ = true;
}

void MetaEditor::movePoint(int pointIndex, double x, double y) {
    auto& pts = host_.model_.metapad.points;
    if (pointIndex >= 0 && pointIndex < (int) pts.size()) {
        pts[(size_t) pointIndex].x = x;
        pts[(size_t) pointIndex].y = y;
        host_.dirty_ = true;
    }
}

void MetaEditor::removePoint(int pointIndex) {
    auto& pts = host_.model_.metapad.points;
    if (pointIndex >= 0 && pointIndex < (int) pts.size()) {
        pts.erase(pts.begin() + pointIndex);
        host_.dirty_ = true;
    }
}

void MetaEditor::setMask(const std::string& organism, int propertyIndex, bool restore) {
    for (auto& e : host_.model_.metapad.mask)
        if (e.organismName == organism && e.propertyIndex == propertyIndex) {
            e.restore = restore; host_.dirty_ = true; return;
        }
    host_.model_.metapad.mask.push_back({organism, propertyIndex, restore});
    host_.dirty_ = true;
}

}
