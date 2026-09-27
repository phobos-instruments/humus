// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/params/RollScope.h"

namespace hum::metascope {

struct Entry {
    std::string organism;
    int property = -1;
    bool restore = true;
};

struct Knob {
    std::string organism;
    int property = -1;
};

enum class Scope { Everything, OnlyPod, WithoutPod };

inline bool has(const std::vector<Entry>& mask, const Knob& k) {
    for (const auto& e : mask)
        if (e.organism == k.organism && e.property == k.property) return true;
    return false;
}

inline std::vector<Entry> completed(std::vector<Entry> mask, const std::vector<Knob>& knobs) {
    for (const auto& k : knobs)
        if (!has(mask, k)) mask.push_back({k.organism, k.property, true});
    return mask;
}

inline std::vector<Entry> tidied(const std::vector<Entry>& mask, const std::vector<Knob>& knobs) {
    std::vector<Entry> kept;
    for (const auto& e : mask) {
        const Knob k{e.organism, e.property};
        bool present = false;
        for (const auto& knob : knobs)
            if (knob.organism == k.organism && knob.property == k.property) { present = true; break; }
        if (present && !has(kept, k)) kept.push_back(e);
    }
    return completed(std::move(kept), knobs);
}
inline std::vector<Entry> scoped(std::vector<Entry> mask, const std::vector<Knob>& knobs, Scope scope,
                                 const std::string& pod) {
    mask = completed(std::move(mask), knobs);
    for (auto& e : mask) {
        const bool inside = rollscope::inPod(e.organism, pod);
        if (scope == Scope::Everything) e.restore = true;
        else if (scope == Scope::OnlyPod) e.restore = inside;
        else if (inside) e.restore = false;
    }
    return mask;
}

enum class PodState { Mixed, AllIn, AllOut, Absent };

inline PodState stateOf(const std::vector<Entry>& mask, const std::string& pod) {
    int in = 0, out = 0;
    for (const auto& e : mask) {
        if (!rollscope::inPod(e.organism, pod)) continue;
        (e.restore ? in : out) += 1;
    }
    if (in + out == 0) return PodState::Absent;
    return out == 0 ? PodState::AllIn : in == 0 ? PodState::AllOut : PodState::Mixed;
}

inline bool onlyPod(const std::vector<Entry>& mask, const std::string& pod) {
    bool any = false;
    for (const auto& e : mask) {
        const bool inside = rollscope::inPod(e.organism, pod);
        if (e.restore != inside) return false;
        any = any || inside;
    }
    return any;
}

}
