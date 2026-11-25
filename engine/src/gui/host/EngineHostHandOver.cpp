// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <cstdlib>
#include <string>
#include <vector>

#include "core/params/ParamSchema.h"

namespace hum {

namespace {

const ParamDesc* descOf(const std::vector<ParamDesc>& schema, const std::string& param) {
    for (const auto& d : schema)
        if (d.name == param) return &d;
    return nullptr;
}

const ParamDesc* takerOf(const std::vector<ParamDesc>& schema, const std::string& kind) {
    for (const auto& d : schema)
        if (!d.takes.empty() && d.takes == kind) return &d;
    return nullptr;
}

bool sharesVar(const std::vector<ParamDesc>& schema, const std::string& name) {
    const auto* d = descOf(schema, name);
    return d != nullptr && d->sharedVar;
}

std::string trimmed(const std::string& s) {
    const auto first = s.find_first_not_of(' ');
    if (first == std::string::npos) return {};
    return s.substr(first, s.find_last_not_of(' ') - first + 1);
}

void applyPicks(EngineHost& host, const std::string& organism, const std::string& picks) {
    size_t at = 0;
    while (at < picks.size()) {
        const auto comma = picks.find(',', at);
        const auto item = picks.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
        if (const auto eq = item.find('='); eq != std::string::npos)
            host.setParam(organism, trimmed(item.substr(0, eq)), std::atof(item.c_str() + eq + 1));
        if (comma == std::string::npos) break;
        at = comma + 1;
    }
}

}

void EngineHost::handOver(const std::string& changed, const std::string& param) {
    if (handingOver_) return;
    bool texts = true;
    if (!changed.empty() && !param.empty()) {
        const auto* cm = model_.byName(changed);
        const auto* d = cm != nullptr ? descOf(schemaFor(cm->classRaw), param) : nullptr;
        if (d == nullptr || (d->handsOver.empty() && !d->sharedVar)) return;
        texts = !d->handsOver.empty();
    }
    handingOver_ = true;
    const DerivedControlScope derived(*this);
    for (const auto& e : mod_.map().entries())
        if (changed.empty() || e.source == changed) handOverRoute(e.source, e.organism, e.param, false, texts);
    handingOver_ = false;
}

void EngineHost::handOverRoute(const std::string& source, const std::string& sink, const std::string& param,
                               bool fresh, bool texts) {
    const auto* to = model_.byName(sink);
    const auto* from = model_.byName(source);
    if (to == nullptr || from == nullptr) return;
    const auto& sinkSchema = schemaFor(to->classRaw);
    const auto* inlet = descOf(sinkSchema, param);
    if (inlet == nullptr || !inlet->handOverInlet) return;
    const auto& sourceSchema = schemaFor(from->classRaw);
    for (const auto& d : sourceSchema) {
        if (!texts || d.handsOver.empty()) continue;
        const auto* taker = takerOf(sinkSchema, d.handsOver);
        if (taker == nullptr) continue;
        const auto text = liveParamText(source, d.name);
        const bool moved = text != liveParamText(sink, taker->name);
        if (moved) setParamText(sink, taker->name, text);
        if (moved || fresh) applyPicks(*this, sink, taker->picks);
    }
    for (const auto& d : sourceSchema) {
        if (!d.sharedVar || !sharesVar(sinkSchema, d.name)) continue;
        const double v = liveParamValue(source, d.name);
        if (v != liveParamValue(sink, d.name)) setParam(sink, d.name, v);
    }
}

}
