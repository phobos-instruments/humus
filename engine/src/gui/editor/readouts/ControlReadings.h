// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>

#include "gui/editor/LiveControls.h"
#include "hum/caps/Audio.h"
#include "hum/caps/Graph.h"

namespace hum::readout {

class ControlValues {
public:
    static constexpr int kMax = 8;

    explicit ControlValues(int decimals) : decimals_(std::max(0, std::min(6, decimals))) {}

    bool poll(ModelHost& host, const std::string& organism) {
        const live::Controls controls(host, organism);
        const int n = std::min(controls.count(), kMax);
        bool changed = n != count_;
        for (int i = 0; i < n && !changed; ++i)
            changed = std::abs(controls[i].value - values_[i]) > 0.5f * std::pow(10.0f, (float) -decimals_)
                   || names_[i] != live::nameOf(controls[i]);
        if (!changed) return false;
        count_ = n;
        for (int i = 0; i < n; ++i) {
            values_[i] = controls[i].value;
            names_[i] = live::nameOf(controls[i]);
        }
        return true;
    }

    int count() const { return count_; }
    int decimals() const { return decimals_; }
    float value(int i) const { return values_[i]; }
    const std::string& name(int i) const { return names_[i]; }

private:
    const int decimals_;
    int count_ = 0;
    float values_[kMax]{};
    std::string names_[kMax];
};

class TextLines {
public:
    static constexpr int kMax = 4;

    bool poll(ModelHost& host, const std::string& organism) {
        std::string fresh[kMax];
        auto* src = live::source<TextSource>(host, organism);
        const int n = src ? src->textLines(fresh, kMax) : 0;
        bool changed = n != count_;
        for (int i = 0; i < n && !changed; ++i) changed = lines_[i] != fresh[i];
        if (!changed) return false;
        count_ = n;
        for (int i = 0; i < n; ++i) lines_[i] = fresh[i];
        return true;
    }

    int count() const { return count_; }
    const std::string& line(int i) const { return lines_[i]; }

private:
    int count_ = 0;
    std::string lines_[kMax];
};

class Led {
public:
    explicit Led(std::string shows) : shows_(std::move(shows)) {}

    bool poll(ModelHost& host, const std::string& organism) {
        const float now = shown(host, organism);
        const bool struck = settled_ && now != last_;
        settled_ = true;
        last_ = now;
        const float was = glow_;
        glow_ = struck ? 1.0f : glow_ * 0.7f;
        if (glow_ < 0.01f) glow_ = 0.0f;
        return std::abs(glow_ - was) > 0.004f;
    }

    float glow() const { return glow_; }

private:
    float shown(ModelHost& host, const std::string& organism) const {
        const live::Controls controls(host, organism);
        if (shows_.empty()) return controls.count() > 0 ? controls[0].value : 0.0f;
        const auto* v = controls.find(shows_);
        return v != nullptr ? v->value : 0.0f;
    }

    const std::string shows_;
    float last_ = 0.0f;
    float glow_ = 0.0f;
    bool settled_ = false;
};

class GainReduction {
public:
    static constexpr float kFullScaleDb = 24.0f;

    bool poll(ModelHost& host, const std::string& organism) {
        const float db = reductionDb(host, organism);
        if (std::abs(db - shown_) <= 0.1f) return false;
        shown_ = db;
        return true;
    }

    static float reductionDb(ModelHost& host, const std::string& organism) {
        if (auto* s = live::source<GainReductionSource>(host, organism)) return s->grDb();
        return 0.0f;
    }

    static float fill(float db) { return std::clamp(db / kFullScaleDb, 0.0f, 1.0f); }

private:
    float shown_ = -1.0f;
};

}
