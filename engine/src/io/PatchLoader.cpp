#include "io/PatchLoader.h"

#include <cstdlib>
#include <unordered_map>

#include "core/AdoptSlot.h"
#include "core/ClassString.h"
#include "core/HostedPlugin.h"
#include "core/PluginNode.h"
#include "core/PluginHost.h"
#include "hum/Registry.h"

namespace hum {

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
        c->loadFrom(OrganismState{cm.properties, cm.pattern});
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

    graph.transport().setLoop(doc.clock.loopStart, doc.clock.loopEnd, doc.clock.loopEnabled);
    {
        const auto& ts = doc.clock.timeSignature;
        const int slash = (int) ts.find('/');
        const int n = ts.empty() ? 0 : std::atoi(slash > 0 ? ts.substr(0, (size_t) slash).c_str() : ts.c_str());
        if (n > 0) graph.transport().setBeatsPerBar((double) n);
    }

    if (graph.nodeCount() == 0) { error = "patch has no organisms"; return false; }
    return true;
}

}
