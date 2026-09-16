// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "core/packs/ClassString.h"
#include "core/plugins/PluginHost.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Video.h"
#include "hum/Registry.h"
#include "io/PatchDocument.h"

namespace hum {

inline std::pair<int, int> classIO(const std::string& cls) {
    OrganismPtr c;
    if (isPluginKind(parseClassString(cls).kind)) c = PluginHost::createOrganism(cls);
    if (!c) c = Registry::instance().create(cls);
    return {c ? c->numAudioInputs() : 0, c ? c->numAudioOutputs() : 0};
}

inline std::pair<int, int> classMidiIO(const std::string& cls) {
    OrganismPtr c;
    if (isPluginKind(parseClassString(cls).kind)) c = PluginHost::createOrganism(cls);
    if (!c) c = Registry::instance().create(cls);
    auto* m = dynamic_cast<MidiNode*>(c.get());
    return {m ? m->numMidiInputs() : 0, m ? m->numMidiOutputs() : 0};
}

inline std::pair<int, int> classVideoIO(const std::string& cls) {
    OrganismPtr c;
    if (isPluginKind(parseClassString(cls).kind)) c = PluginHost::createOrganism(cls);
    if (!c) c = Registry::instance().create(cls);
    auto* v = dynamic_cast<VideoNode*>(c.get());
    return {v ? v->numVideoInputs() : 0, v ? v->numVideoOutputs() : 0};
}

template <class Remove, class Connect>
inline void spliceBefore(const std::vector<ConnectionModel>& conns, const std::string& name,
                  const std::string& nn, std::pair<int, int> io, int nameIns,
                  Remove remove, Connect connect) {
    if (io.first <= 0 && io.second <= 0) return;
    std::vector<ConnectionModel> feeders;
    for (const auto& c : conns) if (c.dst == name) feeders.push_back(c);
    for (const auto& f : feeders) {
        remove(f.src, f.srcOutlet, name, f.dstInlet);
        if (io.first > 0) connect(f.src, f.srcOutlet, nn, std::min(f.dstInlet, io.first - 1));
    }
    for (int k = 0; k < std::min(io.second, nameIns); ++k) connect(nn, k, name, k);
}

template <class Remove, class Connect>
inline void spliceAfter(const std::vector<ConnectionModel>& conns, const std::string& name,
                 const std::string& nn, std::pair<int, int> io, int nameOuts,
                 Remove remove, Connect connect) {
    if (io.first <= 0 && io.second <= 0) return;
    std::vector<ConnectionModel> consumers;
    for (const auto& c : conns) if (c.src == name) consumers.push_back(c);
    for (const auto& u : consumers) {
        remove(name, u.srcOutlet, u.dst, u.dstInlet);
        if (io.second > 0) connect(nn, std::min(u.srcOutlet, io.second - 1), u.dst, u.dstInlet);
    }
    for (int k = 0; k < std::min(nameOuts, io.first); ++k) connect(name, k, nn, k);
}

}
