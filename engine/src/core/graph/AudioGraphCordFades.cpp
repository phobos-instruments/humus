// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/graph/AudioGraph.h"

#include <algorithm>
#include <string>

#include "core/graph/RtWord.h"

namespace hum {

namespace {

bool sameCord(const AudioGraph::Cord& a, const AudioGraph& ga, const AudioGraph::Cord& b, const AudioGraph& gb) {
    if (a.srcOutlet != b.srcOutlet || a.dstInlet != b.dstInlet) return false;
    const Organism* as = ga.organism(a.srcNode);
    const Organism* ad = ga.organism(a.dstNode);
    const Organism* bs = gb.organism(b.srcNode);
    const Organism* bd = gb.organism(b.dstNode);
    return as != nullptr && ad != nullptr && bs != nullptr && bd != nullptr
        && as->name() == bs->name() && ad->name() == bd->name();
}

bool hasCord(const AudioGraph& g, const AudioGraph::Cord& c, const AudioGraph& from) {
    const auto& cords = g.cords();
    return std::any_of(cords.begin(), cords.end(),
                       [&](const AudioGraph::Cord& other) { return sameCord(other, g, c, from); });
}

}

void AudioGraph::sumCord(Cord& c, const float* s, float* d, int numSamples) {
    const float target = rtLoadWord(c.target);
    float gain = rtLoadWord(c.gain);
    if (gain == target) {
        if (gain == 1.0f)
            for (int i = 0; i < numSamples; ++i) d[i] += s[i];
        else if (gain != 0.0f)
            for (int i = 0; i < numSamples; ++i) d[i] += s[i] * gain;
        return;
    }
    for (int i = 0; i < numSamples; ++i) {
        gain += std::clamp(target - gain, -cordStep_, cordStep_);
        d[i] += s[i] * gain;
    }
    rtStoreWord(c.gain, gain);
}

void AudioGraph::rampInCordsNewTo(const AudioGraph& old) {
    for (auto& c : cords_)
        if (!hasCord(old, c, *this)) c.gain = 0.0f;
}

std::vector<int> AudioGraph::cordsMissingFrom(const AudioGraph& next) const {
    std::vector<int> missing;
    for (int i = 0; i < (int) cords_.size(); ++i)
        if (!hasCord(next, cords_[(size_t) i], *this)) missing.push_back(i);
    return missing;
}

void AudioGraph::fadeOutCords(const std::vector<int>& cords) {
    for (int i : cords)
        if (i >= 0 && i < (int) cords_.size()) rtStoreWord(cords_[(size_t) i].target, 0.0f);
}

bool AudioGraph::cordsFaded(const std::vector<int>& cords) const {
    for (int i : cords)
        if (i >= 0 && i < (int) cords_.size() && rtLoadWord(cords_[(size_t) i].gain) > 0.0f) return false;
    return true;
}

}
