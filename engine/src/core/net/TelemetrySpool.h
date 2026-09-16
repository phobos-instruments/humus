// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class TelemetrySpool {
public:
    void setEnabled(bool e) { enabled_ = e; }
    bool enabled() const { return enabled_; }

    void count(const std::string& name, juce::int64 n = 1) {
        if (enabled_ && !name.empty()) counts_[name] += n;
    }
    void setCrashedLastRun(bool c) { crashed_ = c; }
    bool crashedLastRun() const { return crashed_; }

    bool empty() const { return counts_.empty() && !crashed_; }

    std::vector<std::pair<std::string, juce::int64>> events() const {
        return {counts_.begin(), counts_.end()};
    }

    juce::String toJson() const {
        auto* o = new juce::DynamicObject();
        o->setProperty("crashed", crashed_);
        auto* c = new juce::DynamicObject();
        for (const auto& [name, n] : counts_)
            c->setProperty(juce::Identifier(juce::String(name)), n);
        o->setProperty("counts", juce::var(c));
        return juce::JSON::toString(juce::var(o), true);
    }

    void addFromJson(const juce::String& json) {
        const auto v = juce::JSON::parse(json);
        if (!v.isObject()) return;
        crashed_ = crashed_ || (bool) v.getProperty("crashed", false);
        const auto c = v.getProperty("counts", juce::var());
        if (auto* obj = c.getDynamicObject())
            for (const auto& p : obj->getProperties())
                counts_[p.name.toString().toStdString()] += (juce::int64) p.value;
    }

    void clearAfterFlush() {
        counts_.clear();
        crashed_ = false;
    }

    static constexpr juce::int64 kFlushIntervalMs = 6ll * 3600 * 1000;
    static bool flushDue(juce::int64 nowMs, juce::int64 lastFlushMs,
                         bool consented, bool empty) {
        if (!consented || empty) return false;
        return nowMs - lastFlushMs >= kFlushIntervalMs;
    }

private:
    std::map<std::string, juce::int64> counts_;
    bool enabled_ = false;
    bool crashed_ = false;
};

}
