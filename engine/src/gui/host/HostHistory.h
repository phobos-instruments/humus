// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "io/HistoryRules.h"
#include "io/PatchDocument.h"
#include "io/PatchHistory.h"

namespace hum {

class EngineHost;

class HostHistory {
public:
    static constexpr std::int64_t kDefaultIntervalMs = 5 * history::kMinuteMs;

    explicit HostHistory(EngineHost& host, PatchHistory store = PatchHistory());

    void begin(const juce::String& documentPath, bool opened, std::int64_t now);
    bool snapshot(const juce::String& documentPath, const juce::String& reason, std::int64_t now,
                  bool evenIfUnchanged = false);
    bool tick(const juce::String& documentPath, std::int64_t now);
    void saved(const juce::String& previousPath, const juce::String& documentPath, std::int64_t now);

    bool restore(const PatchHistory::Entry& entry, const juce::String& documentPath, std::int64_t now,
                 std::string& error);
    bool openCopy(const PatchHistory::Entry& entry, const juce::String& originalPath, std::int64_t now,
                  std::string& error);

    std::vector<PatchHistory::Entry> entries() const { return store_.entries(key_); }
    bool keep(std::int64_t at, const juce::String& name) { return store_.keep(key_, at, name); }
    bool release(std::int64_t at) { return store_.release(key_, at); }
    int clear(const juce::String& documentPath, std::int64_t now);
    const juce::String& key() const { return key_; }

    std::int64_t intervalMs = kDefaultIntervalMs;

private:
    void rebase(std::int64_t now);

    EngineHost& host_;
    PatchHistory store_;
    juce::String key_;
    PatchDocumentModel base_;
    juce::uint64 stamp_ = 0;
    std::int64_t lastTick_ = 0;
};

}
