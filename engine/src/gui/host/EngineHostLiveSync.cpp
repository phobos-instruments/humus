// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "io/ModRouteBuild.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "core/params/ParamSchema.h"
#include "core/graph/PerfBox.h"
#include "gui/editor/ControlDefaults.h"
#include "core/graph/RtWord.h"

#include "gui/host/AutomationLanes.h"

namespace hum {

using namespace lanes;

void EngineHost::syncRoutes() {
    if (!graph_) return;
    std::vector<ModRoute> routes;
    for (const auto& e : mod_.map().entries()) {
        const auto* target = model_.byName(e.organism);
        if (target == nullptr || !routeLivesInEngine(*target, e.param)) continue;
        ModRoute r;
        r.dstParam = e.param;
        if (fillModRoute(r, *target, model_.byName(e.source), e.value, e.min, e.max, e.shape,
                         [&](const std::string& n) { return graph_->indexOf(n); }))
            routes.push_back(std::move(r));
    }
    {
        const juce::ScopedLock sl(lock_);
        graph_->setModRoutes(std::move(routes));
    }
    pushSocketSources();
}

void EngineHost::pushSocketSources() {
    if (!graph_) return;
    for (auto& cm : model_.organisms) {
        auto* sink = dynamic_cast<SocketSources*>(graph_->find(cm.name));
        if (sink == nullptr) continue;
        for (const auto& d : schemaFor(cm.classRaw)) {
            if (!d.socket) continue;
            std::string source;
            for (const auto& e : mod_.map().entries()) {
                if (e.organism != cm.name || e.param != d.name || isParamSource(e.value)) continue;
                const auto* tagged = dynamic_cast<const Tagged*>(graph_->find(e.source));
                source = tagged != nullptr && !tagged->tag().empty() ? tagged->tag() : e.source;
            }
            sink->setSocketSource(d.name, source);
        }
    }
}

void EngineHost::syncAutomation() {
    if (!graph_) return;
    std::vector<AutoLane> lanes;
    bool amountLane = false, gridLane = false;
    for (auto& cm : model_.organisms) {
        const bool clockPseudo = isClockPseudo(cm.displayClass);
        const int node = clockPseudo ? -1 : graph_->indexOf(cm.name);
        if (node < 0 && !clockPseudo) continue;
        for (auto& ml : cm.automation) {
            const int laneNode = clockPseudo ? clockLaneNode(ml.propertyName) : node;
            if (laneNode == -1) continue;
            const bool live = !ml.mute && !ml.points.empty();
            amountLane = amountLane || (live && laneNode == AutoLane::kGrooveNode);
            gridLane = gridLane || (live && laneNode == AutoLane::kGrooveGridNode);
            AutoLane lane;
            lane.node = laneNode;
            lane.param = ml.propertyName;
            lane.mute = ml.mute;
            lane.suspended = playing_ && touched_.count({cm.name, ml.propertyName}) > 0;
            lane.kind = ml.kind == "range" ? AutoKind::Range
                      : ml.kind == "trigger" ? AutoKind::Trigger
                      : ml.kind == "step" ? AutoKind::Step : AutoKind::Double;
            if (lane.kind == AutoKind::Trigger)
                for (const auto& d : schemaFor(cm.classRaw))
                    if (d.name == ml.propertyName) {
                        lane.rest = d.min;
                        lane.pulse = d.max;
                        break;
                    }
            for (auto& bp : ml.points)
                lane.points.push_back({bp.beat, bp.value, bp.valueMax, bp.curve});
            lanes.push_back(std::move(lane));
        }
    }
    const juce::ScopedLock sl(lock_);
    graph_->setAutomation(std::move(lanes));
    graph_->setMeterMap(meterMapOf(model_));
    restoreUnautomatedGroove(amountLane, gridLane);
}

OrganismModel* EngineHost::mutableByName(const std::string& name) {
    for (auto& c : model_.organisms) if (c.name == name) return &c;
    return nullptr;
}

void EngineHost::syncPattern(const std::string& name) {
    if (patternBatch_ > 0) { patternPending_.insert(name); return; }
    auto* cm = mutableByName(name);
    if (!cm) return;
    Organism* live = nullptr;
    {
        const juce::ScopedLock sl(lock_);
        if (graph_)
            if (auto* c = graph_->find(name)) { c->setPattern(cm->pattern); live = c; }
    }
    if (auto* cr = dynamic_cast<ClipRecorder*>(live)) cr->ensureClipsLoaded();
    syncNodeTrack(name);
}

bool EngineHost::nodeIsNoteTrack(const std::string& name) {
    if (graph_ == nullptr) return false;
    auto* c = graph_->find(name);
    if (c == nullptr) return false;
    if (dynamic_cast<ClipArrangement*>(c) != nullptr) return false;
    auto* mn = dynamic_cast<MidiNode*>(c);
    if (mn == nullptr || mn->numMidiInputs() <= 0) return false;
    const auto* cm = model_.byName(name);
    if (cm == nullptr) return false;
    for (const auto& ch : cm->pattern.channels)
        if (ch.type == "note-events") return true;
    return false;
}

void EngineHost::syncNodeTrack(const std::string& name) {
    if (graph_ == nullptr) return;
    const int idx = graph_->indexOf(name);
    if (idx < 0) return;
    std::vector<noteschedule::Voice> voices;
    if (nodeIsNoteTrack(name))
        if (const auto* cm = model_.byName(name))
            voices = noteschedule::prepare(cm->pattern, 4 * 4 * Pattern::kTicksPerBeat);
    if (std::getenv("HUMUS_NOTE_DEBUG")) {
        int notes = 0, chans = 0;
        if (const auto* cm = model_.byName(name))
            for (const auto& ch : cm->pattern.channels)
                if (ch.type == "note-events") { ++chans; notes += (int) decodeNoteEvents(ch.matrix).size(); }
        std::fprintf(stderr, "[note] %s: %d voices, %d note-chans, %d notes\n",
                     name.c_str(), (int) voices.size(), chans, notes);
    }
    if (std::getenv("HUMUS_NOTE_DEBUG"))
        std::fprintf(stderr, "[note] %s: track holds %d at peak\n", name.c_str(),
                     graph_->maxTrackHeld());
    if (voices.empty() && !graph_->hasNodeTrack(idx)) return;
    const juce::ScopedLock sl(lock_);
    graph_->setNodeTrack(idx, std::move(voices));
    if (const auto* cm = model_.byName(name))
        graph_->setNodeTrackMuted(idx, modelTrackMuted(*cm));
}

void EngineHost::syncMeter() {
    const juce::ScopedLock sl(lock_);
    if (graph_) graph_->setMeterMap(meterMapOf(model_));
}

}
