// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <mutex>
#include <string>

namespace hum {

class NamedValues {
public:
    struct Entry {
        float value = 0.0f;
        unsigned stamp = 0;
    };

    static NamedValues& instance() {
        static NamedValues v;
        return v;
    }

    void post(const std::string& name, float value) {
        if (name.empty()) return;
        const std::lock_guard<std::mutex> l(lock_);
        auto& e = entries_[name];
        e.value = value;
        e.stamp = ++counter_;
    }

    bool take(const std::string& name, unsigned& seenStamp, float& value) const {
        if (name.empty()) return false;
        const std::lock_guard<std::mutex> l(lock_);
        const auto it = entries_.find(name);
        if (it == entries_.end() || it->second.stamp == seenStamp) return false;
        seenStamp = it->second.stamp;
        value = it->second.value;
        return true;
    }

private:
    mutable std::mutex lock_;
    std::map<std::string, Entry> entries_;
    unsigned counter_ = 0;
};

}
