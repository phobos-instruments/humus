// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "gui/editor/LiveControls.h"
#include "hum/caps/Audio.h"

namespace hum::readout {

struct TraceColumn {
    float low = 0.0f, high = 0.0f;
};

inline int triggerStart(const float* wave, int count, int window, float level) {
    const int latest = count - window;
    if (latest <= 0) return 0;
    for (int i = latest; i >= 1; --i)
        if (wave[i - 1] < level && wave[i] >= level) return i;
    return latest;
}

inline void traceColumns(const float* wave, int count, int columns, std::vector<TraceColumn>& out) {
    out.assign((size_t) std::max(0, columns), TraceColumn{});
    if (count <= 0 || columns <= 0) return;
    for (int c = 0; c < columns; ++c) {
        const int from = (int) ((long long) c * count / columns);
        const int to = std::max(from + 1, (int) ((long long) (c + 1) * count / columns));
        TraceColumn col{wave[from], wave[from]};
        for (int i = from; i < std::min(to, count); ++i) {
            col.low = std::min(col.low, wave[i]);
            col.high = std::max(col.high, wave[i]);
        }
        out[(size_t) c] = col;
    }
}

inline float beamBrightness(float travel) {
    return 1.0f / (1.0f + 0.35f * std::max(0.0f, travel));
}

class Trace {
public:
    static constexpr int kSearchSamples = 4096;
    static constexpr int kAfterglowFrames = 14;

    bool poll(ModelHost& host, const std::string& organism) {
        auto* src = live::source<TraceSource>(host, organism);
        if (src == nullptr) return false;
        shape_ = src->traceShape();
        const unsigned stamp = src->traceStamp();
        const bool fresh = stamp != lastStamp_;
        lastStamp_ = stamp;
        if (shape_.held) return false;
        if (!fresh) {
            if (afterglow_ <= 0) return false;
            --afterglow_;
            fading_ = true;
            return true;
        }
        afterglow_ = kAfterglowFrames;
        capture(*src);
        return true;
    }

    const TraceSource::Shape& shape() const { return shape_; }
    const std::vector<float>& left() const { return left_; }
    const std::vector<float>& right() const { return right_; }
    bool fading() const { return fading_; }

    void load(const TraceSource::Shape& shape, std::vector<float> left, std::vector<float> right) {
        shape_ = shape;
        frame(std::move(left), std::move(right));
    }

private:
    void capture(const TraceSource& src) {
        const int window = std::max(1, shape_.windowSamples);
        std::vector<float> l((size_t) (window + kSearchSamples)), r(l.size());
        const int got = src.traceRead(l.data(), r.data(), (int) l.size());
        l.resize((size_t) got);
        r.resize((size_t) got);
        frame(std::move(l), std::move(r));
    }

    void frame(std::vector<float> l, std::vector<float> r) {
        fading_ = false;
        const int count = (int) l.size();
        const int window = std::min(count, std::max(1, shape_.windowSamples));
        int start = count - window;
        if (!shape_.plotsLeftAgainstRight) {
            std::vector<float> sum((size_t) count);
            for (int i = 0; i < count; ++i) sum[(size_t) i] = 0.5f * (l[(size_t) i] + r[(size_t) i]);
            start = triggerStart(sum.data(), count, window, 0.0f);
        }
        left_.assign(l.begin() + start, l.begin() + start + window);
        right_.assign(r.begin() + start, r.begin() + start + window);
        if (!shape_.sumsToMono) return;
        for (size_t i = 0; i < left_.size(); ++i) left_[i] = 0.5f * (left_[i] + right_[i]);
        right_ = left_;
    }

    TraceSource::Shape shape_;
    std::vector<float> left_, right_;
    unsigned lastStamp_ = 0;
    int afterglow_ = 0;
    bool fading_ = false;
};

}
