// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/PatchLoader.h"

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/graph/AdoptSlot.h"
#include "core/library/BankLibrary.h"
#include "core/packs/ClassString.h"
#include "core/plugins/HostedPlugin.h"
#include "core/plugins/PluginNode.h"
#include "core/plugins/PluginHost.h"
#include "core/params/ParamSchema.h"
#include "hum/Registry.h"
#include "io/MeterMap.h"
#include "io/ModRouteBuild.h"
#include "hum/Swing.h"

namespace hum {

namespace {

bool isLibraryRef(const std::string& text) {
    return text.rfind(banks::kLegacyPrefix, 0) == 0 || text.rfind(kAssetScheme, 0) == 0;
}

std::string dspText(const std::string& text, const OrganismModel& cm) {
    return isLibraryRef(text) ? banks::resolve(text, cm.displayClass) : text;
}

bool carriesLibraryRef(const OrganismModel& cm) {
    for (const auto& p : cm.properties)
        if (isLibraryRef(p.text)) return true;
    return false;
}

void seedAbsentParams(Organism& c, const OrganismModel& cm) {
    const std::string& cls = cm.classRaw.empty() ? cm.displayClass : cm.classRaw;
    int idx = 0;
    for (const auto& d : schemaFor(cls)) {
        const int at = idx++;
        if (c.params.byName(d.name) != nullptr) continue;
        Parameter p;
        p.index = at;
        p.name = d.name;
        if (d.isText) {
            p.type = d.isPlainText ? "text" : "soundfile";
            p.text = dspText(d.text, cm);
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
            p.value = d.def;
        }
        c.params.add(p);
    }
}

void fillLibraryDefaults(Organism& c, const OrganismModel& cm) {
    const std::string& cls = cm.classRaw.empty() ? cm.displayClass : cm.classRaw;
    for (const auto& d : schemaFor(cls)) {
        if (!d.isText || !isLibraryRef(d.text)) continue;
        if (auto* p = c.params.byName(d.name); p != nullptr && p->text.empty())
            p->text = dspText(d.text, cm);
    }
}

}

bool buildGraph(const PatchDocumentModel& doc, AudioGraph& graph, std::string& error,
                const std::function<const Organism*(const OrganismModel&)>& reuseLookup) {
    registerBuiltinOrganisms();
    std::unordered_map<std::string, int> nodeIndex;

    for (const auto& cm : doc.organisms) {
        OrganismPtr c;
        if (isPluginKind(cm.kind)) {
            if (reuseLookup)
                if (const Organism* live = reuseLookup(cm))
                    c = std::make_unique<AdoptSlot>(*live);
            if (!c) {
                c = PluginHost::createOrganism(cm.classRaw);
                if (c && !cm.pluginState.empty())
                    if (auto* hp = dynamic_cast<PluginNode*>(c.get()))
                        hp->setStateBase64(cm.pluginState);
            }
        }
        if (!c) c = Registry::instance().create(cm.displayClass);
        c->setName(cm.name);
        const bool refs = carriesLibraryRef(cm);
        std::vector<Parameter> resolved;
        if (refs) {
            resolved = cm.properties;
            for (auto& p : resolved) p.text = dspText(p.text, cm);
        }
        c->loadFrom(OrganismState{refs ? resolved : cm.properties, cm.pattern});
        if (!isPluginKind(cm.kind)) {
            seedAbsentParams(*c, cm);
            fillLibraryDefaults(*c, cm);
        }
        const int idx = graph.addNode(std::move(c));
        nodeIndex[cm.name] = idx;
        if (modelBypassed(cm)) graph.setNodeBypass(idx, true);
    }

    for (const auto& conn : doc.connections) {
        auto s = nodeIndex.find(conn.src);
        auto d = nodeIndex.find(conn.dst);
        if (s == nodeIndex.end() || d == nodeIndex.end()) continue;
        graph.connect(s->second, conn.srcOutlet, d->second, conn.dstInlet);
    }
    for (const auto& conn : doc.midiConnections) {
        auto s = nodeIndex.find(conn.src);
        auto d = nodeIndex.find(conn.dst);
        if (s == nodeIndex.end() || d == nodeIndex.end()) continue;
        graph.connectMidi(s->second, conn.srcOutlet, d->second, conn.dstInlet, conn.midiChannel);
    }

    std::vector<AutoLane> lanes;
    for (const auto& cm : doc.organisms) {
        auto it = nodeIndex.find(cm.name);
        if (it == nodeIndex.end()) continue;
        for (const auto& ml : cm.automation) {
            AutoLane lane;
            lane.node = it->second;
            lane.param = ml.propertyName;
            lane.mute = ml.mute;
            lane.kind = ml.kind == "range" ? AutoKind::Range
                      : ml.kind == "trigger" ? AutoKind::Trigger : AutoKind::Double;
            if (lane.kind == AutoKind::Trigger)
                for (const auto& d : schemaFor(cm.classRaw))
                    if (d.name == ml.propertyName) {
                        lane.rest = d.min;
                        lane.pulse = d.max;
                        break;
                    }
            for (const auto& bp : ml.points)
                lane.points.push_back({bp.beat, bp.value, bp.valueMax, bp.curve});
            lanes.push_back(std::move(lane));
        }
    }
    graph.setAutomation(std::move(lanes));
    graph.setModRoutes(modRoutesFor(doc.organisms, [&](const std::string& n) {
        auto it = nodeIndex.find(n);
        return it == nodeIndex.end() ? -1 : it->second;
    }));

    graph.transport().setLoop(doc.clock.loopStart, doc.clock.loopEnd, doc.clock.loopEnabled);
    graph.setMeterMap(meterMapOf(doc));
    graph.transport().setGroove(swing::grooveFor(doc.groove, doc.grooveUnit));

    if (graph.nodeCount() == 0) { error = "patch has no organisms"; return false; }
    return true;
}

}
