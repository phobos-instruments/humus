// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <limits>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class AutosaveInPlace {
public:
    static constexpr std::int64_t kQuietMs = 5000;
    static constexpr const char* kSettingKey = "autosave.inPlace";

    struct Moment {
        bool hasFile = false;
        bool dirty = false;
        bool busy = false;
        std::uint64_t stamp = 0;
    };

    bool due(const Moment& m, std::int64_t now) {
        if (!m.hasFile || !m.dirty || m.stamp != stamp_) {
            stamp_ = m.stamp;
            since_ = now;
            return false;
        }
        return !m.busy && m.stamp != failed_ && now - since_ >= kQuietMs;
    }

    void failed(std::uint64_t stamp) { failed_ = stamp; }

    static bool canWrite(const juce::File& patch, const std::vector<juce::File>& shippedRoots,
                         const juce::File& app) {
        if (!patch.existsAsFile() || !patch.hasWriteAccess()) return false;
        if (app != juce::File() && patch.isAChildOf(app)) return false;
        for (const auto& root : shippedRoots)
            if (patch.isAChildOf(root)) return false;
        return true;
    }

private:
    std::uint64_t stamp_ = 0;
    std::uint64_t failed_ = std::numeric_limits<std::uint64_t>::max();
    std::int64_t since_ = 0;
};

}
