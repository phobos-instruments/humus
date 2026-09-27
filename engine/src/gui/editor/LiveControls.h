// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>

#include "gui/host/ModelHost.h"
#include "hum/Organism.h"
#include "hum/caps/Graph.h"

namespace hum::live {

template <class Source>
Source* source(ModelHost& host, const std::string& organism) {
    return dynamic_cast<Source*>(host.liveOrganism(organism));
}

inline std::string nameOf(const ControlSource::ControlVal& v) { return v.name != nullptr ? v.name : ""; }

class Controls {
public:
    static constexpr int kMax = 16;

    explicit Controls(const ControlSource* src)
        : count_(src != nullptr ? std::clamp(src->controlValues(vals_, kMax), 0, kMax) : 0) {}
    Controls(ModelHost& host, const std::string& organism) : Controls(source<ControlSource>(host, organism)) {}

    int count() const { return count_; }
    const ControlSource::ControlVal& operator[](int i) const { return vals_[i]; }

    const ControlSource::ControlVal* find(const std::string& name) const {
        for (int i = 0; i < count_; ++i)
            if (nameOf(vals_[i]) == name) return &vals_[i];
        return nullptr;
    }

    float valueOr(const std::string& name, float fallback) const {
        const auto* v = find(name);
        return v != nullptr ? v->value : fallback;
    }

private:
    ControlSource::ControlVal vals_[kMax];
    int count_ = 0;
};

}
