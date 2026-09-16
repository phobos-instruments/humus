// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <utility>

#include "hum/Parameter.h"

namespace hum {

class ParamRef {
public:
    ParamRef() = default;
    explicit ParamRef(std::string name) : name_(std::move(name)) {}

    static ParamRef numbered(const std::string& prefix, int n, const std::string& suffix = {}) {
        return ParamRef(prefix + std::to_string(n) + suffix);
    }

    const std::string& name() const { return name_; }

    double get(const ParameterSet& ps, double fallback) const {
        const auto* p = find(ps);
        if (p == nullptr) return fallback;
        return p->isRange ? p->rangeMin : p->value;
    }
    bool on(const ParameterSet& ps, double fallback = 0.0) const {
        return get(ps, fallback) >= 0.5;
    }
    const std::string& text(const ParameterSet& ps, const std::string& fallback) const {
        const auto* p = find(ps);
        return p == nullptr ? fallback : p->text;
    }

private:
    const Parameter* find(const ParameterSet& ps) const {
        if (slot_ < 0) slot_ = ps.slotOf(name_);
        return ps.slot(slot_);
    }

    std::string name_;
    mutable int slot_ = -1;
};

template <std::size_t N>
std::array<ParamRef, N> numberedParams(const std::string& prefix, const std::string& suffix = {}) {
    std::array<ParamRef, N> out;
    for (std::size_t i = 0; i < N; ++i) out[i] = ParamRef::numbered(prefix, (int) i + 1, suffix);
    return out;
}

}
