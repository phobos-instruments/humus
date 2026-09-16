// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/graph/AudioGraph.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstddef>
#include <queue>
#include <typeinfo>
#include "core/graph/AdoptSlot.h"
#include "core/tuning/MtsTuning.h"
#include "core/plugins/PluginNode.h"
#include "core/graph/RtWord.h"
#include "hum/Swing.h"
#include "hum/dsp/DspMath.h"

namespace hum {

void AudioGraph::resolveTunings() {
    const int n = (int) nodes_.size();
    for (auto& nd : nodes_) nd.tuning = nullptr;
    defaultTuning_ = nullptr;

    std::vector<std::vector<int>> succ((size_t) n);
    for (const auto& c : midiCords_)
        if (c.srcNode != c.dstNode) succ[(size_t) c.srcNode].push_back(c.dstNode);

    for (int i = 0; i < n; ++i) {
        auto* tp = nodes_[(size_t) i].tuningProvider;
        if (tp == nullptr) continue;
        if (succ[(size_t) i].empty()) {
            if (defaultTuning_ == nullptr) defaultTuning_ = tp;
            continue;
        }
        std::vector<char> seen((size_t) n, 0);
        std::vector<int> stack = succ[(size_t) i];
        while (!stack.empty()) {
            const int v = stack.back();
            stack.pop_back();
            if (seen[(size_t) v]) continue;
            seen[(size_t) v] = 1;
            Node& d = nodes_[(size_t) v];
            if (d.tuningProvider != nullptr) continue;
            if (d.tuning != nullptr) continue;
            d.tuning = &tp->tuning();
            for (int w : succ[(size_t) v]) stack.push_back(w);
        }
    }
    transport_.setActiveTuning(defaultTuning_ != nullptr ? &defaultTuning_->tuning() : nullptr);
}

void AudioGraph::pumpPluginTunings() {
    const Tuning* def = defaultTuning_ != nullptr ? &defaultTuning_->tuning() : nullptr;
    static const Tuning kStandard;
    for (auto& nd : nodes_) {
        if (nd.plugin == nullptr) continue;
        const Tuning& active = nd.tuning != nullptr ? *nd.tuning
                             : def != nullptr       ? *def
                                                    : kStandard;
        if (active.pluginDelivery() == Tuning::kDeliverBend) continue;
        if (active.pluginDelivery() == Tuning::kDeliverAuto && nd.bendVerdict) continue;
        if (active == nd.sentTuning) continue;
        for (const auto& m : mtsRetuneMessages(active)) nd.plugin->queueMidiMessage(m);
        nd.sentTuning = active;
    }
}

std::vector<std::string> AudioGraph::pluginsNeedingVerdict() const {
    std::vector<std::string> out;
    const Tuning* def = defaultTuning_ != nullptr ? &defaultTuning_->tuning() : nullptr;
    for (const auto& nd : nodes_) {
        if (nd.plugin == nullptr || nd.verdictKnown) continue;
        const Tuning* act = nd.tuning != nullptr ? nd.tuning : def;
        if (act == nullptr || act->isStandard()) continue;
        if (act->pluginDelivery() != Tuning::kDeliverAuto) continue;
        const auto& cls = nd.plugin->classRaw();
        bool dup = false;
        for (const auto& c : out) dup |= c == cls;
        if (!dup) out.push_back(cls);
    }
    return out;
}

void AudioGraph::setBendVerdict(const std::string& classRaw, bool needsBend) {
    for (auto& nd : nodes_) {
        if (nd.plugin == nullptr || nd.plugin->classRaw() != classRaw) continue;
        nd.bendVerdict = needsBend;
        nd.verdictKnown = true;
        nd.sentTuning = Tuning();
    }
}

}
