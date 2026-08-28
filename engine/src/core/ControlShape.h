#pragma once
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace hum {

struct ControlShape {
    double smoothing = 0.0;
    std::vector<std::pair<double, double>> curve;
    bool isSwitch = false;
    bool inverted = false;
    bool toggle = false;
    double threshold = 0.5;
    bool logScale = false;

    bool isDefault() const {
        return smoothing <= 0.0 && curve.empty() && !isSwitch && !inverted && !toggle
            && !logScale;
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
};

inline double advanceControlShape(const ControlShape& s, ControlShapeState& st,
                                  double t, double dt) {
    t = std::min(1.0, std::max(0.0, t));
    double out;
    if (s.isSwitch) {
        const bool above = t > s.threshold;
        if (st.lastOut < 0.0) {
            st.on = s.toggle ? false : (above != s.inverted);
        } else if (s.toggle) {
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

inline bool rangeIsLogarithmic(double lo, double hi) {
    return lo > 0.0 && hi / lo >= 50.0;
}

inline double shapedToRange(const ControlShape& s, double min, double max, double shaped) {
    if (s.logScale && min > 0.0 && max > 0.0)
        return min * std::pow(max / min, shaped);
    return min + (max - min) * shaped;
}

}
