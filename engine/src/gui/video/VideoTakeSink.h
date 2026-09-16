// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class VideoTakeSink {
public:
    virtual ~VideoTakeSink() = default;
    virtual void pushFrame(const std::uint8_t* bottomUpRgba, int w, int h, double beat,
                           double tempo, bool rolling) = 0;
};

class VideoTakeStore {
public:
    struct Entry {
        std::string node;
        std::shared_ptr<VideoTakeSink> sink;
        int w = 0, h = 0;
    };

    static VideoTakeStore& instance() {
        static VideoTakeStore s;
        return s;
    }

    void open(const std::string& node, std::shared_ptr<VideoTakeSink> sink, int w, int h) {
        const juce::ScopedLock sl(lock_);
        entries_[node] = {node, std::move(sink), w, h};
    }

    void close(const std::string& node) {
        const juce::ScopedLock sl(lock_);
        entries_.erase(node);
    }

    std::vector<Entry> entries() const {
        const juce::ScopedLock sl(lock_);
        std::vector<Entry> out;
        for (const auto& [n, e] : entries_) out.push_back(e);
        return out;
    }

    bool empty() const {
        const juce::ScopedLock sl(lock_);
        return entries_.empty();
    }

private:
    juce::CriticalSection lock_;
    std::map<std::string, Entry> entries_;
};

}
