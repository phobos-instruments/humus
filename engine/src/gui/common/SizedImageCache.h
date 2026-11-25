// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <list>
#include <map>
#include <utility>

#include <juce_graphics/juce_graphics.h>

namespace hum {

class SizedImageCache {
public:
    explicit SizedImageCache(size_t budgetBytes, juce::uint32 idleMs = 0) : budget_(budgetBytes), idleMs_(idleMs) {}

    juce::Image find(juce::int64 key, juce::uint32 now = juce::Time::getMillisecondCounter()) {
        sweep(now);
        const auto it = entries_.find(key);
        if (it == entries_.end()) return {};
        it->second.used = now;
        recency_.splice(recency_.begin(), recency_, it->second.place);
        return it->second.image;
    }

    juce::Image put(juce::int64 key, juce::Image image, juce::uint32 now = juce::Time::getMillisecondCounter()) {
        drop(key);
        sweep(now);
        const size_t cost = bytesOf(image);
        while (!recency_.empty() && bytes_ + cost > budget_) drop(recency_.back());
        recency_.push_front(key);
        entries_.emplace(key, Entry{image, recency_.begin(), cost, now});
        bytes_ += cost;
        return image;
    }

    void sweep(juce::uint32 now = juce::Time::getMillisecondCounter()) {
        if (idleMs_ == 0) return;
        while (!recency_.empty() && now - entries_.at(recency_.back()).used > idleMs_) drop(recency_.back());
    }

    size_t bytes() const { return bytes_; }
    size_t count() const { return entries_.size(); }

    static size_t bytesOf(const juce::Image& image) {
        return image.isValid() ? (size_t) image.getWidth() * (size_t) image.getHeight() * 4u : 0u;
    }

private:
    struct Entry {
        juce::Image image;
        std::list<juce::int64>::iterator place;
        size_t bytes = 0;
        juce::uint32 used = 0;
    };

    void drop(juce::int64 key) {
        const auto it = entries_.find(key);
        if (it == entries_.end()) return;
        bytes_ -= it->second.bytes;
        recency_.erase(it->second.place);
        entries_.erase(it);
    }

    std::map<juce::int64, Entry> entries_;
    std::list<juce::int64> recency_;
    size_t bytes_ = 0;
    size_t budget_;
    juce::uint32 idleMs_;
};

}
