// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>

namespace hum {

class TapTempoCore {
public:
    double tap(double nowSeconds) {
        if (last_ >= 0.0) {
            const double gap = nowSeconds - last_;
            if (gap < 0.02) return msOrZero();
            const double avg = averageSeconds();
            if (gap > 3.0 || (avg > 0.0 && gap > avg * 3.0)) {
                intervals_.clear();
            } else {
                if (intervals_.size() >= 4) intervals_.erase(intervals_.begin());
                intervals_.push_back(gap);
            }
        }
        last_ = nowSeconds;
        return msOrZero();
    }

    void reset() {
        intervals_.clear();
        last_ = -1.0;
    }

private:
    double averageSeconds() const {
        if (intervals_.empty()) return 0.0;
        double s = 0.0;
        for (double v : intervals_) s += v;
        return s / (double) intervals_.size();
    }
    double msOrZero() const {
        const double a = averageSeconds();
        return a > 0.0 ? a * 1000.0 : 0.0;
    }

    std::vector<double> intervals_;
    double last_ = -1.0;
};

}
