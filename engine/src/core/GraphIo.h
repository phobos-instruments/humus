#pragma once
#include <vector>

#include <juce_core/juce_core.h>

#include "core/AudioGraph.h"
#include "hum/Capabilities.h"

namespace hum {

inline MasterTap* findMasterTap(AudioGraph& g) {
    for (int i = 0; i < g.nodeCount(); ++i)
        if (auto* tap = dynamic_cast<MasterTap*>(g.organism(i))) return tap;
    return nullptr;
}

inline std::vector<MasterTap*> findMasterTaps(AudioGraph& g) {
    std::vector<MasterTap*> taps;
    for (int i = 0; i < g.nodeCount(); ++i)
        if (auto* tap = dynamic_cast<MasterTap*>(g.organism(i))) taps.push_back(tap);
    return taps;
}

inline std::vector<HardwareOut*> findHardwareOuts(AudioGraph& g) {
    std::vector<HardwareOut*> outs;
    for (int i = 0; i < g.nodeCount(); ++i)
        if (auto* h = dynamic_cast<HardwareOut*>(g.organism(i))) outs.push_back(h);
    return outs;
}

inline void buildChannelMap(const juce::BigInteger& activeChannels, int* map, int maxChannels) {
    int packed = 0;
    for (int p = 0; p < maxChannels; ++p)
        map[p] = activeChannels[p] ? packed++ : -1;
}

inline void renderGraphBlock(AudioGraph& g, const float* const* in, int numIn,
                             float* const* out, int numOut, int numSamples,
                             const std::vector<MasterTap*>& masters,
                             const std::vector<HardwareOut*>& auxes) {
    g.transport().setLiveInput(in, numIn, numSamples);
    g.processBlock(numSamples);
    g.transport().setLiveInput(nullptr, 0, 0);

    for (auto* master : masters) {
        const int first = master->firstChannel();
        for (int c = 0; c < master->channels(); ++c) {
            const int dst = first + c;
            if (dst < 0 || dst >= numOut) continue;
            const float* src = master->channelData(c);
            for (int i = 0; i < numSamples; ++i) out[dst][i] += src[i];
        }
    }
    for (auto* a : auxes) {
        const int ch = a->channel();
        if (ch < 0 || ch >= numOut) continue;
        const float* src = a->channelData();
        const int n = std::min(a->blockLength(), numSamples);
        for (int i = 0; i < n; ++i) out[ch][i] += src[i];
    }
}

}
