// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "core/midi/ControlMode.h"

namespace hum {

struct ControlShape {
    ControlType type = ControlType::Fader;
    ButtonMode button = ButtonMode::Hold;
    FaderMode fader = FaderMode::Direct;
    EncoderFormat encoder = EncoderFormat::TwosComplement;
    double step = kDefaultControlStep;
    double smoothing = 0.0;
    std::vector<std::pair<double, double>> curve;
    bool inverted = false;
    double threshold = 0.5;
    bool logScale = false;

    bool isButton() const { return type == ControlType::Button; }
    bool isEncoder() const { return type == ControlType::Encoder; }
    bool isFader() const { return type == ControlType::Fader; }
    bool picksUp() const { return isFader() && fader == FaderMode::PickUp; }

    bool isDefault() const {
        return isFader() && fader == FaderMode::Direct && smoothing <= 0.0 && curve.empty()
            && !inverted && !logScale;
    }

    double curveAt(double t) const {
        t = std::min(1.0, std::max(0.0, t));
        if (curve.size() < 2) return t;
        if (t <= curve.front().first) return curve.front().second;
        for (size_t i = 1; i < curve.size(); ++i) {
            if (t > curve[i].first) continue;
            const auto& a = curve[i - 1];
            const auto& b = curve[i];
            const double span = b.first - a.first;
            const double f = span > 1e-12 ? (t - a.first) / span : 1.0;
            return a.second + (b.second - a.second) * f;
        }
        return curve.back().second;
    }
};

struct ControlShapeState {
    double smoothed = -1.0;
    double lastOut = -1.0;
    bool wasAbove = false;
    bool on = false;
    bool seen = false;
    bool wasPressed = false;
    bool caught = false;
    double lastShaped = -1.0;
    double lastWritten = -1.0;
};

enum class ControlActionKind { None, Absolute, Delta, Reset };

struct ControlAction {
    ControlActionKind kind = ControlActionKind::None;
    double value = 0.0;
    bool none() const { return kind == ControlActionKind::None; }
};

struct ControlUpdate {
    std::string organism;
    std::string param;
    double value = 0.0;
    ControlActionKind kind = ControlActionKind::Absolute;
    double rangeMin = 0.0;
    double rangeMax = 1.0;
    bool logScale = false;
};

inline double advanceControlShape(const ControlShape& s, ControlShapeState& st,
                                  double t, double dt) {
    t = std::min(1.0, std::max(0.0, t));
    double out;
    if (s.isButton()) {
        const bool toggle = s.button == ButtonMode::Toggle;
        const bool above = t > s.threshold;
        if (st.lastOut < 0.0) {
            st.on = toggle ? false : (above != s.inverted);
        } else if (toggle) {
            if (above != st.wasAbove && above == !s.inverted) st.on = !st.on;
        } else {
            st.on = above != s.inverted;
        }
        st.wasAbove = above;
        out = st.on ? 1.0 : 0.0;
    } else {
        if (st.smoothed < 0.0) st.smoothed = t;
        else if (s.smoothing > 1e-6 && dt > 0.0) {
            st.smoothed += (t - st.smoothed) * (1.0 - std::exp(-dt / s.smoothing));
            if (std::abs(t - st.smoothed) < 1e-4) st.smoothed = t;
        } else {
            st.smoothed = t;
        }
        out = s.curveAt(st.smoothed);
    }
    if (st.lastOut >= 0.0 && std::abs(out - st.lastOut) < 1e-9) return -1.0;
    st.lastOut = out;
    return out;
}

inline bool pressEdge(const ControlShape& s, ControlShapeState& st, double t) {
    const bool pressed = (t > s.threshold) != s.inverted;
    const bool edge = pressed && !(st.seen && st.wasPressed);
    st.seen = true;
    st.wasPressed = pressed;
    return edge;
}

inline ControlAction advanceControl(const ControlShape& s, ControlShapeState& st,
                                    double t, double dt) {
    if (s.isButton() && !buttonModeIsAbsolute(s.button)) {
        if (!pressEdge(s, st, t)) return {};
        switch (s.button) {
            case ButtonMode::StepUp:   return {ControlActionKind::Delta, s.step};
            case ButtonMode::StepDown: return {ControlActionKind::Delta, -s.step};
            case ButtonMode::Reset:    return {ControlActionKind::Reset, 0.0};
            default: return {};
        }
    }
    if (s.isButton() && s.button == ButtonMode::Set) {
        if (!pressEdge(s, st, t)) return {};
        return {ControlActionKind::Absolute, 1.0};
    }
    const double shaped = advanceControlShape(s, st, t, dt);
    if (shaped < 0.0) return {};
    return {ControlActionKind::Absolute, shaped};
}

inline ControlAction encoderAction(const ControlShape& s, int ticks) {
    if (ticks == 0) return {};
    return {ControlActionKind::Delta, ticks * s.step * (s.inverted ? -1.0 : 1.0)};
}

inline constexpr double kPickUpWindow = 0.02;

inline bool pickUpAllows(ControlShapeState& st, double shaped, double current) {
    if (st.lastWritten >= 0.0 && std::abs(current - st.lastWritten) > kPickUpWindow) st.caught = false;
    if (!st.caught) {
        const bool near = std::abs(shaped - current) <= kPickUpWindow;
        const bool crossed = st.lastShaped >= 0.0
                             && (st.lastShaped - current) * (shaped - current) <= 0.0;
        st.caught = near || crossed;
    }
    st.lastShaped = shaped;
    if (st.caught) st.lastWritten = shaped;
    return st.caught;
}

inline bool rangeIsLogarithmic(double lo, double hi) {
    return lo > 0.0 && hi / lo >= 50.0;
}

inline double shapedToRange(const ControlShape& s, double min, double max, double shaped) {
    if (s.logScale && min > 0.0 && max > 0.0)
        return min * std::pow(max / min, shaped);
    return min + (max - min) * shaped;
}

inline double rangeToShaped(const ControlShape& s, double min, double max, double value) {
    if (s.logScale && min > 0.0 && max > 0.0 && value > 0.0 && std::abs(max - min) > 1e-12)
        return std::log(value / min) / std::log(max / min);
    return std::abs(max - min) > 1e-12 ? (value - min) / (max - min) : 0.0;
}

inline ControlUpdate controlUpdate(const std::string& organism, const std::string& param,
                                   const ControlShape& s, double min, double max,
                                   const ControlAction& a) {
    ControlUpdate u{organism, param, a.value, a.kind, min, max, s.logScale};
    if (a.kind == ControlActionKind::Absolute) u.value = shapedToRange(s, min, max, a.value);
    return u;
}

}
