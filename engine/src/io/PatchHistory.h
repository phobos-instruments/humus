// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class PatchHistory {
public:
    struct Entry {
        juce::File file;
        std::int64_t at = 0;
        juce::String reason;
        juce::String summary;
        juce::String name;
        bool kept = false;
    };

    using Writer = std::function<bool(const juce::File&)>;

    explicit PatchHistory(juce::File root = defaultRoot());

    static juce::File defaultRoot();
    static juce::String keyFor(const juce::String& documentPath);
    static juce::String untitledKey(std::int64_t now);

    juce::File folderOf(const juce::String& key) const;
    std::vector<Entry> entries(const juce::String& key) const;
    bool add(const juce::String& key, std::int64_t at, const Writer& write, const juce::String& reason,
             const juce::String& summary);
    bool keep(const juce::String& key, std::int64_t at, const juce::String& name);
    bool release(const juce::String& key, std::int64_t at);
    int thin(const juce::String& key, std::int64_t now);
    int clear(const juce::String& key);
    void adopt(const juce::String& from, const juce::String& to);

private:
    static juce::File metaFor(const juce::File& snapshot);
    static void writeMeta(const Entry& e);

    juce::File root_;
};

}
