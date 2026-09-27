// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "gui/editor/LiveControls.h"
#include "hum/caps/Audio.h"
#include "hum/dsp/DspMath.h"
#include "hum/dsp/LevelMeter.h"

namespace hum::readout {

class LevelBars {
public:
    static float fillFor(float level) {
        const float db = 20.0f * std::log10(std::max(level, 1.0e-5f));
        return std::clamp((db + 48.0f) / 48.0f, 0.0f, 1.0f);
    }
    static float levelForFill(float fill) {
        return std::pow(10.0f, (std::clamp(fill, 0.0f, 1.0f) * 48.0f - 48.0f) / 20.0f);
    }
    static bool moved(float a, float b) { return std::abs(fillFor(a) - fillFor(b)) > 0.02f; }
    static int cellsFor(int width) { return std::clamp(width / 6, 8, 24); }
    static int litCells(float level, int width) { return (int) std::lround(fillFor(level) * cellsFor(width)); }

    bool poll(ModelHost& host, const std::string& organism, bool inlet = false) {
        float lv[LevelMeter::kMax];
        const int n = inlet ? host.nodeInletMeter(organism, lv, LevelMeter::kMax)
                            : host.nodeMeter(organism, lv, LevelMeter::kMax);
        bool dirty = (int) levels_.size() != n;
        levels_.resize((size_t) n);
        for (int c = 0; c < n; ++c) {
            if (moved(lv[c], levels_[(size_t) c])) dirty = true;
            levels_[(size_t) c] = lv[c];
        }
        return dirty;
    }

    const std::vector<float>& levels() const { return levels_; }

private:
    std::vector<float> levels_;
};

class Threshold {
public:
    Threshold(ModelHost& host, std::string organism, std::string param)
        : host_(host), organism_(std::move(organism)), param_(std::move(param)), last_(value()) {}

    float value() const { return (float) host_.liveParamValue(organism_, param_); }
    bool off() const { return value() <= 0.0f; }
    float fill() const { return LevelBars::fillFor(value()); }
    float decibels() const { return 20.0f * std::log10(value()); }

    bool poll() {
        const float t = value();
        if (std::abs(t - last_) <= 1.0e-6f) return false;
        last_ = t;
        return true;
    }

    void begin() { host_.beginParamDrag(organism_, param_); }
    bool dragToFill(float fill) {
        const float t = fill <= 0.03f ? 0.0f : std::clamp(LevelBars::levelForFill(fill), 0.0f, 1.0f);
        if (std::abs(t - value()) < 1.0e-6f) return false;
        host_.editParam(organism_, param_, (double) t);
        return true;
    }
    void end() { host_.endParamDrag(); }

private:
    ModelHost& host_;
    std::string organism_, param_;
    float last_ = 0.0f;
};

class VuNeedles {
public:
    static constexpr float kHoldS = 1.5f;
    static constexpr float kLampTrip = 0.8913f;

    bool poll(ModelHost& host, const std::string& organism, bool live, unsigned nowMs) {
        auto* src = live::source<VuSource>(host, organism);
        const float dt = 1.0f / 30.0f;
        bool moving = false;
        for (int c = 0; c < 2; ++c) {
            const float pk = src != nullptr && live ? src->vuPeak(c) : 0.0f;
            const float db = 20.0f * std::log10(std::max(pk, 1.0e-5f));
            const float target = pk <= 0.0f ? 0.0f : std::clamp((db + 48.0f) / 48.0f, 0.0f, 1.04f);
            const float w = kTwoPiF * 2.1f;
            const float z = 0.62f;
            vel_[c] += (w * w * (target - pos_[c]) - 2.0f * z * w * vel_[c]) * dt;
            pos_[c] += vel_[c] * dt;
            if (pk >= kLampTrip) lampUntil_[c] = nowMs + (unsigned) (kHoldS * 1000.0f);
            const bool lit = nowMs < lampUntil_[c];
            const float lampTarget = lit ? 1.0f : 0.0f;
            lamp_[c] += (lampTarget - lamp_[c]) * (lit ? 1.0f : 0.28f);
            if (target <= 0.0f && std::abs(pos_[c]) < 0.004f && std::abs(vel_[c]) < 0.01f && lamp_[c] < 0.01f) {
                pos_[c] = 0.0f;
                vel_[c] = 0.0f;
                lamp_[c] = 0.0f;
            }
            if (std::abs(pos_[c] - shown_[c]) > 0.0025f || lamp_[c] > 0.01f) moving = true;
        }
        if (!moving) return false;
        shown_[0] = pos_[0];
        shown_[1] = pos_[1];
        return true;
    }

    float needle(int channel) const { return pos_[channel]; }
    float lamp(int channel) const { return lamp_[channel]; }

private:
    float pos_[2] = {0, 0}, vel_[2] = {0, 0}, shown_[2] = {0, 0};
    float lamp_[2] = {0, 0};
    unsigned lampUntil_[2] = {0, 0};
};

}
