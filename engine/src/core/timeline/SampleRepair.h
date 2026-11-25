// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum::samplefix {

struct PencilPoint {
    std::int64_t sample = 0;
    float value = 0.0f;
};

struct Span {
    std::int64_t start = 0;
    std::vector<float> values;
};

inline Span pencilSpan(const std::vector<PencilPoint>& points) {
    Span span;
    if (points.empty()) return span;
    std::int64_t lo = points.front().sample, hi = lo;
    for (const auto& p : points) {
        lo = std::min(lo, p.sample);
        hi = std::max(hi, p.sample);
    }
    span.start = lo;
    span.values.assign((std::size_t) (hi - lo + 1), 0.0f);
    span.values[(std::size_t) (points.front().sample - lo)] = points.front().value;
    for (std::size_t i = 1; i < points.size(); ++i) {
        const auto& a = points[i - 1];
        const auto& b = points[i];
        const std::int64_t steps = std::llabs(b.sample - a.sample);
        const std::int64_t dir = b.sample >= a.sample ? 1 : -1;
        for (std::int64_t k = 0; k <= steps; ++k) {
            const float t = steps == 0 ? 1.0f : (float) k / (float) steps;
            span.values[(std::size_t) (a.sample + dir * k - lo)] = a.value + (b.value - a.value) * t;
        }
    }
    return span;
}

namespace detail {

inline constexpr int kMaxContext = 2048;
inline constexpr int kMinContext = 16;
inline constexpr int kMaxOrder = 32;

inline bool burg(const std::vector<double>& x, int order, std::vector<double>& coef) {
    const int n = (int) x.size();
    if (n < 2 || order < 1 || order >= n) return false;
    coef.assign((std::size_t) order + 1, 0.0);
    std::vector<double> wk1(x.begin(), x.end() - 1), wk2(x.begin() + 1, x.end()), wkm((std::size_t) order + 1, 0.0);
    for (int k = 1; k <= order; ++k) {
        double num = 0.0, denom = 0.0;
        for (int j = 0; j < n - k; ++j) {
            num += wk1[(std::size_t) j] * wk2[(std::size_t) j];
            denom += wk1[(std::size_t) j] * wk1[(std::size_t) j] + wk2[(std::size_t) j] * wk2[(std::size_t) j];
        }
        if (!(denom > 1.0e-30)) return false;
        coef[(std::size_t) k] = 2.0 * num / denom;
        for (int i = 1; i < k; ++i)
            coef[(std::size_t) i] = wkm[(std::size_t) i] - coef[(std::size_t) k] * wkm[(std::size_t) (k - i)];
        if (k == order) return true;
        for (int i = 1; i <= k; ++i) wkm[(std::size_t) i] = coef[(std::size_t) i];
        for (int j = 0; j < n - k - 1; ++j) {
            wk1[(std::size_t) j] -= wkm[(std::size_t) k] * wk2[(std::size_t) j];
            wk2[(std::size_t) j] = wk2[(std::size_t) j + 1] - wkm[(std::size_t) k] * wk1[(std::size_t) j + 1];
        }
    }
    return true;
}

inline bool predict(std::vector<double> history, int count, std::vector<double>& out) {
    const int order = std::max(2, std::min(kMaxOrder, (int) history.size() / 4));
    std::vector<double> coef;
    if (!burg(history, order, coef)) return false;
    out.assign((std::size_t) count, 0.0);
    for (int i = 0; i < count; ++i) {
        double v = 0.0;
        const std::size_t end = history.size();
        for (int k = 1; k <= order; ++k) v += coef[(std::size_t) k] * history[end - (std::size_t) k];
        if (!std::isfinite(v)) return false;
        out[(std::size_t) i] = v;
        history.push_back(v);
    }
    return true;
}

inline std::vector<float> cubic(const float* x, std::int64_t length, std::int64_t start, int count) {
    const std::int64_t end = start + count;
    const bool left = start > 0, right = end < length;
    const double p0 = left ? x[start - 1] : (right ? x[end] : 0.0);
    const double p1 = right ? x[end] : p0;
    const double m0 = start > 1 ? x[start - 1] - x[start - 2] : 0.0;
    const double m1 = end + 1 < length ? x[end + 1] - x[end] : 0.0;
    double peak = std::max(std::abs(p0), std::abs(p1));
    for (std::int64_t i = std::max<std::int64_t>(0, start - 64); i < start; ++i) peak = std::max(peak, (double) std::abs(x[i]));
    for (std::int64_t i = end; i < std::min(length, end + 64); ++i) peak = std::max(peak, (double) std::abs(x[i]));
    const double span = count + 1.0;
    std::vector<float> out((std::size_t) count);
    for (int i = 0; i < count; ++i) {
        const double t = (i + 1.0) / span, t2 = t * t, t3 = t2 * t;
        const double v = (2.0 * t3 - 3.0 * t2 + 1.0) * p0 + (t3 - 2.0 * t2 + t) * span * m0
                       + (-2.0 * t3 + 3.0 * t2) * p1 + (t3 - t2) * span * m1;
        out[(std::size_t) i] = (float) std::clamp(v, -peak, peak);
    }
    return out;
}

}

inline std::vector<float> repairGap(const float* channel, std::int64_t length, std::int64_t gapStart, int gapLength) {
    if (channel == nullptr || length <= 0) return {};
    const std::int64_t start = std::clamp<std::int64_t>(gapStart, 0, length);
    const std::int64_t end = std::clamp<std::int64_t>(gapStart + std::max(0, gapLength), start, length);
    const int count = (int) (end - start);
    if (count <= 0) return {};
    const int leftContext = (int) std::min<std::int64_t>(detail::kMaxContext, start);
    const int rightContext = (int) std::min<std::int64_t>(detail::kMaxContext, length - end);
    if (leftContext < detail::kMinContext || rightContext < detail::kMinContext)
        return detail::cubic(channel, length, start, count);
    std::vector<double> left(channel + start - leftContext, channel + start);
    std::vector<double> right((std::size_t) rightContext);
    for (int i = 0; i < rightContext; ++i) right[(std::size_t) i] = channel[end + rightContext - 1 - i];
    std::vector<double> forward, backward;
    if (!detail::predict(left, count, forward) || !detail::predict(right, count, backward))
        return detail::cubic(channel, length, start, count);
    std::vector<float> out((std::size_t) count);
    for (int i = 0; i < count; ++i) {
        const double t = (i + 1.0) / (count + 1.0);
        const double w = 0.5 + 0.5 * std::cos(hum::kPi * t);
        const double v = forward[(std::size_t) i] * w + backward[(std::size_t) (count - 1 - i)] * (1.0 - w);
        if (!std::isfinite(v)) return detail::cubic(channel, length, start, count);
        out[(std::size_t) i] = (float) v;
    }
    return out;
}

}
