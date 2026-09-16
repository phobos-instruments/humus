// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/ModHost.h"
#include "hum/Organism.h"

#include <algorithm>
#include <cmath>

#include "core/params/ParamSchema.h"
#include "gui/editor/ControlDefaults.h"
#include "hum/caps/Graph.h"
#include "io/ModRouteBuild.h"

namespace hum {

namespace {
bool readParam(BrickHost& host, const std::string& organism, const std::string& param,
               float& out) {
    const auto* cm = host.model().byName(organism);
    if (cm == nullptr) return false;
    for (const auto& d : schemaFor(cm->classRaw)) {
        if (d.name != param) continue;
        if (d.max <= d.min) return false;
        out = (float) std::clamp((host.liveParamValue(organism, param) - d.min)
                                     / (d.max - d.min), 0.0, 1.0);
        return true;
    }
    return false;
}

void widenEmptyRange(BrickHost& host, const std::string& organism, const std::string& param,
                     double& min, double& max) {
    if (std::abs(max - min) > 1.0e-12) return;
    const auto* cm = host.model().byName(organism);
    if (cm == nullptr) return;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == param && d.max > d.min) {
            if (d.carry) return;
            min = d.min;
            max = d.max;
            return;
        }
}

ControlShape defaultShapeFor(BrickHost& host, const std::string& organism,
                             const std::string& param) {
    ControlShape d;
    if (paramIsSwitch(host, organism, param)) d.isSwitch = true;
    else d.logScale = paramIsLog(host, organism, param);
    return d;
}

bool readSource(BrickHost& host, const std::string& source, const std::string& value,
                float& out) {
    if (isParamSource(value)) return readParam(host, source, paramSourceName(value), out);
    auto* src = dynamic_cast<ControlSource*>(host.liveOrganism(source));
    if (src == nullptr) return false;
    ControlSource::ControlVal vals[32];
    const int n = src->controlValues(vals, 32);
    for (int i = 0; i < n; ++i)
        if (value == vals[i].name) { out = ControlSource::unitOf(vals[i]); return true; }
    return false;
}
}

void ModHost::mapRoute(const std::string& source, const std::string& value,
                       const std::string& organism, const std::string& param,
                       double min, double max) {
    if (source == organism && paramSourceName(value) == param && isParamSource(value)) return;
    widenEmptyRange(host_, organism, param, min, max);
    map_.set(source, value, organism, param, min, max);
    if (const auto* sh = map_.shapeOf(source, value, organism, param))
        if (sh->isDefault())
            if (const auto d = defaultShapeFor(host_, organism, param); !d.isDefault())
                map_.setShape(source, value, organism, param, d);
    syncMapToModel();
    nodes_.syncRoutes();
    doc_.markDirty();
    doc_.pokeLiveRefresh();
}

void ModHost::clearRoute(const std::string& source, const std::string& value,
                         const std::string& organism, const std::string& param) {
    map_.clear(source, value, organism, param);
    syncMapToModel();
    nodes_.syncRoutes();
    doc_.markDirty();
    doc_.pokeLiveRefresh();
}

void ModHost::clearForOrganism(const std::string& organism) {
    map_.clearOrganism(organism);
}

void ModHost::renameOrganism(const std::string& oldName, const std::string& newName) {
    map_.renameOrganism(oldName, newName);
}

void ModHost::setShape(const std::string& source, const std::string& value,
                       const std::string& organism, const std::string& param,
                       const ControlShape& shape) {
    map_.setShape(source, value, organism, param, shape);
    syncMapToModel();
    nodes_.syncRoutes();
    doc_.markDirty();
}

std::vector<ModParamUpdate> ModHost::tick(double dt) {
    if (map_.empty()) return {};
    return map_.tick(
        [this](const std::string& source, const std::string& value, float& out) {
            return readSource(host_, source, value, out);
        },
        dt,
        [this](const ModMapEntry& e) {
            const auto* target = host_.model().byName(e.organism);
            return target != nullptr && routeLivesInEngine(*target, e.param);
        });
}

std::vector<std::pair<std::string, std::string>> ModHost::availableSources() const {
    std::vector<std::pair<std::string, std::string>> out;
    for (const auto& cm : host_.model().organisms) {
        if (auto* src = dynamic_cast<ControlSource*>(host_.liveOrganism(cm.name))) {
            ControlSource::ControlVal vals[32];
            const int n = src->controlValues(vals, 32);
            for (int i = 0; i < n; ++i) out.push_back({cm.name, vals[i].name});
        }
        for (const auto& d : schemaFor(cm.classRaw))
            if (!d.isText && !d.isTrigger && d.max > d.min)
                out.push_back({cm.name, std::string(kParamSource) + d.name});
    }
    return out;
}

void ModHost::syncMapFromModel() {
    map_.clearAll();
    for (auto& c : doc_.document().organisms)
        for (auto& s : c.modSources) {
            widenEmptyRange(host_, c.name, s.propertyName, s.mapMin, s.mapMax);
            map_.set(s.sourceOrganism, s.sourceValue, c.name, s.propertyName, s.mapMin,
                     s.mapMax);
            ControlShape sh;
            sh.smoothing = s.smoothing;
            sh.curve = s.curve;
            sh.isSwitch = s.isSwitch || isHostSwitchTarget(s.propertyName);
            sh.inverted = s.inverted;
            sh.toggle = s.toggle;
            sh.threshold = s.threshold;
            if (!sh.isSwitch) sh.logScale = paramIsLog(host_, c.name, s.propertyName);
            if (!sh.isDefault())
                map_.setShape(s.sourceOrganism, s.sourceValue, c.name, s.propertyName, sh);
        }
}

void ModHost::syncMapToModel() {
    for (auto& c : doc_.document().organisms) c.modSources.clear();
    for (const auto& e : map_.entries()) {
        auto* cm = doc_.mutableByName(e.organism);
        if (!cm) continue;
        ModControllerSource s;
        s.propertyName = e.param;
        s.propertyIndex = -1;
        for (auto& pr : cm->properties)
            if (pr.name == e.param) { s.propertyIndex = pr.index; break; }
        s.sourceOrganism = e.source;
        s.sourceValue = e.value;
        s.mapMin = e.min;
        s.mapMax = e.max;
        s.smoothing = e.shape.smoothing;
        s.curve = e.shape.curve;
        s.isSwitch = e.shape.isSwitch;
        s.inverted = e.shape.inverted;
        s.toggle = e.shape.toggle;
        s.threshold = e.shape.threshold;
        cm->modSources.push_back(std::move(s));
    }
}

}
