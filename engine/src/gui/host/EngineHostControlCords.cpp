// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <algorithm>
#include "gui/host/EngineHost.h"
#include "io/ModRouteBuild.h"
#include "core/params/ParamSchema.h"
#include "core/packs/ClassString.h"
#include "core/packs/PackRegistry.h"
#include "core/packs/Roles.h"
#include "hum/caps/Graph.h"
#include "hum/Registry.h"

namespace hum {

static bool bypassesPodBoundary(const PatchDocumentModel& m, const std::string& src, int outlet,
                                const std::string& dst, int inlet, pods::Domain dom) {
    auto check = [&](const std::string& inner, int chan, const std::string& outer, bool isDst) {
        for (std::string pod = pods::parentOf(inner); !pod.empty();
             pod = pods::parentOf(pod)) {
            if (pods::isUnder(outer, pod) || outer == pod) break;
            const auto declared = dom == pods::Domain::Midi
                                      ? pods::midiDeclaredRefs(m, pod, isDst)
                                  : dom == pods::Domain::Video ? pods::videoDeclaredRefs(m, pod, isDst)
                                                               : pods::declaredRefs(m, pod, isDst);
            if (pods::indexOfRef(declared, inner, chan) < 0) return true;
        }
        return false;
    };
    return check(src, outlet, dst, false) || check(dst, inlet, src, true);
}

bool EngineHost::cordWouldStray(const std::string& src, int outlet, const std::string& dst,
                                int inlet, pods::Domain dom) const {
    return bypassesPodBoundary(model_, src, outlet, dst, inlet, dom);
}

bool EngineHost::socketShown(const std::string& name, const std::string& param) const {
    for (const auto& e : mod_.map().entries())
        if (e.organism == name && e.param == param) return true;
    const auto* pins = graph_ ? dynamic_cast<const PinKinds*>(graph_->find(name)) : nullptr;
    return pins == nullptr || pins->controlInlet(param);
}

int EngineHost::controlInletsOf(const std::string& name) const {
    if (pods::isPod(model_, name)) return (int) pods::controlPortRefs(model_, name, true).size();
    const auto* cm = model_.byName(name);
    if (cm == nullptr) return 0;
    int n = 0;
    for (const auto& d : schemaFor(cm->classRaw)) n += d.socket && socketShown(name, d.name) ? 1 : 0;
    return n;
}

std::string EngineHost::controlInletParam(const std::string& name, int inlet) const {
    if (pods::isPod(model_, name))
        return inlet >= 0 && inlet < controlInletsOf(name) ? pods::kControlPortParam : std::string();
    const auto* cm = model_.byName(name);
    if (cm == nullptr) return {};
    int n = 0;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.socket && socketShown(name, d.name) && n++ == inlet) return d.name;
    return {};
}

int EngineHost::controlOutletsOf(const std::string& name) {
    if (pods::isPod(model_, name)) return (int) pods::controlPortRefs(model_, name, false).size();
    auto* live = liveNode(name);
    const auto* pins = dynamic_cast<const PinKinds*>(live);
    const auto* src = dynamic_cast<const ControlSource*>(live);
    if (pins == nullptr || src == nullptr) return 0;
    ControlSource::ControlVal vals[32];
    const int n = src->controlValues(vals, 32);
    int outs = 0;
    for (int i = 0; i < n; ++i) outs += pins->controlOutlet(i) ? 1 : 0;
    return outs;
}

std::string EngineHost::controlOutletValue(const std::string& name, int outlet) {
    if (pods::isPod(model_, name))
        return outlet >= 0 && outlet < controlOutletsOf(name) ? pods::kControlPortValue : std::string();
    auto* live = liveNode(name);
    const auto* pins = dynamic_cast<const PinKinds*>(live);
    const auto* src = dynamic_cast<const ControlSource*>(live);
    if (pins == nullptr || src == nullptr) return {};
    ControlSource::ControlVal vals[32];
    const int n = src->controlValues(vals, 32);
    int k = 0;
    for (int i = 0; i < n; ++i)
        if (pins->controlOutlet(i) && k++ == outlet) return vals[i].name;
    return {};
}

bool EngineHost::controlEndpoint(std::string& node, int& pin, bool isDstSide) const {
    if (!pods::isPod(model_, node)) return true;
    const auto ports = pods::controlPortRefs(model_, node, isDstSide);
    if (pin < 0 || pin >= (int) ports.size()) return false;
    node = ports[(size_t) pin];
    pin = 0;
    return true;
}

std::vector<EngineHost::ControlCord> EngineHost::controlCordsInScope(const std::string& scope) {
    std::vector<ControlCord> out;
    for (const auto& c : controlCords()) {
        const auto s = pods::controlSurfaceOf(model_, c.src, c.srcOutlet, false, scope);
        const auto d = pods::controlSurfaceOf(model_, c.dst, c.dstInlet, true, scope);
        if (s.kind == pods::Surface::None || d.kind == pods::Surface::None) continue;
        const std::string sn = s.kind == pods::Surface::Box ? s.box : c.src;
        const std::string dn = d.kind == pods::Surface::Box ? d.box : c.dst;
        if (sn == dn) continue;
        out.push_back({sn, s.pin, dn, d.pin});
    }
    return out;
}

void EngineHost::connectControl(const std::string& srcIn, int outletIn, const std::string& dstIn, int inletIn) {
    std::string src = srcIn, dst = dstIn;
    int outlet = outletIn, inlet = inletIn;
    if (!controlEndpoint(src, outlet, false) || !controlEndpoint(dst, inlet, true)) return;
    if (src == dst) return;
    const auto value = controlOutletValue(src, outlet);
    const auto param = controlInletParam(dst, inlet);
    if (value.empty() || param.empty()) return;
    double lo = 0.0, hi = 0.0;
    if (const auto* cm = model_.byName(dst))
        if (const auto* d = paramDescOf(*cm, param))
            if (!d->carry) socketDefaultRange(*d, lo, hi);
    pushUndo();
    mod_.mapRoute(src, value, dst, param, lo, hi);
    if (onTopologyChanged) onTopologyChanged();
}

void EngineHost::disconnectControl(const std::string& srcIn, int outletIn, const std::string& dstIn, int inletIn) {
    std::string src = srcIn, dst = dstIn;
    int outlet = outletIn, inlet = inletIn;
    if (!controlEndpoint(src, outlet, false) || !controlEndpoint(dst, inlet, true)) return;
    const auto value = controlOutletValue(src, outlet);
    const auto param = controlInletParam(dst, inlet);
    if (value.empty() || param.empty()) return;
    pushUndo();
    mod_.clearRoute(src, value, dst, param);
    if (onTopologyChanged) onTopologyChanged();
}

std::vector<EngineHost::ControlCord> EngineHost::controlCords() {
    std::vector<ControlCord> out;
    for (const auto& e : mod_.map().entries()) {
        if (isParamSource(e.value)) continue;
        int dstInlet = -1;
        for (int i = 0, n = controlInletsOf(e.organism); i < n; ++i)
            if (controlInletParam(e.organism, i) == e.param) { dstInlet = i; break; }
        if (dstInlet < 0) continue;
        int srcOutlet = -1;
        for (int i = 0, n = controlOutletsOf(e.source); i < n; ++i)
            if (controlOutletValue(e.source, i) == e.value) { srcOutlet = i; break; }
        if (srcOutlet < 0) continue;
        out.push_back({e.source, srcOutlet, e.organism, dstInlet});
    }
    return out;
}

}
