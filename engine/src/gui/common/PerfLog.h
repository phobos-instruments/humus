// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

#include <juce_core/juce_core.h>

namespace hum::perf {

inline bool enabled() {
    static const bool on = std::getenv("HUM_PERF") != nullptr;
    return on;
}

struct Bucket {
    double ms = 0.0;
    int calls = 0;
};

inline std::map<std::string, Bucket>& buckets() {
    static std::map<std::string, Bucket> b;
    return b;
}

inline void flush() {
    static double last = juce::Time::getMillisecondCounterHiRes();
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (now - last < 2000.0) return;
    const double window = now - last;
    last = now;
    std::fprintf(stderr, "[perf] over %.0f ms:", window);
    for (auto& [label, b] : buckets()) {
        if (b.calls == 0) continue;
        std::fprintf(stderr, "  %s %.0fms/%d", label.c_str(), b.ms, b.calls);
        b = {};
    }
    std::fprintf(stderr, "\n");
}

struct Scope {
    const char* label;
    double t0;
    explicit Scope(const char* l)
        : label(l), t0(enabled() ? juce::Time::getMillisecondCounterHiRes() : 0.0) {}
    ~Scope() {
        if (!enabled()) return;
        auto& b = buckets()[label];
        b.ms += juce::Time::getMillisecondCounterHiRes() - t0;
        ++b.calls;
        flush();
    }
};

}
