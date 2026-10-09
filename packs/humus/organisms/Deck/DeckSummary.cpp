// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Deck/DeckSummary.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace hum {

namespace {

std::vector<float> buildPeaks(const DeckAudio& audio, int buckets) {
    if (audio.numChannels <= 0 || audio.length <= 0) return {};
    std::vector<float> peaks((std::size_t) buckets, 0.0f);
    const double per = (double) audio.length / (double) buckets;
    for (int b = 0; b < buckets; ++b) {
        const auto s0 = (std::int64_t) (b * per);
        const auto s1 = std::min(audio.length, (std::int64_t) ((b + 1) * per));
        float peak = 0.0f;
        for (int c = 0; c < audio.numChannels; ++c)
            for (auto s = s0; s < s1; ++s) peak = std::max(peak, std::abs(audio.channels[c][s]));
        peaks[(std::size_t) b] = std::min(1.0f, peak);
    }
    return peaks;
}

struct SummaryMemo {
    std::string stamp;
    DeckSummary summary;
};

constexpr std::size_t kSummaryMemoSlots = 8;

}

DeckSummary summaryFor(const std::string& stamp, const DeckAudio& audio, int buckets) {
    static std::mutex lock;
    static std::vector<SummaryMemo> memo;
    {
        const std::lock_guard<std::mutex> held(lock);
        for (const auto& m : memo)
            if (m.stamp == stamp) return m.summary;
    }
    DeckSummary made{buildPeaks(audio, buckets),
                     scanLoudness(audio.channels, audio.numChannels, audio.length, audio.sampleRate)};
    const std::lock_guard<std::mutex> held(lock);
    if (memo.size() >= kSummaryMemoSlots) memo.erase(memo.begin());
    memo.push_back({stamp, made});
    return made;
}

}
