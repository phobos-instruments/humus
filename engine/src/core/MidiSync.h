#pragma once
#include <cmath>

namespace hum {

class MidiClockGenerator {
public:
    int ticksFor(double beats) {
        const double tickPos = std::floor(beats * 24.0);
        if (!primed_ || tickPos < lastTick_) {
            lastTick_ = tickPos;
            primed_ = true;
            return 0;
        }
        int n = (int) (tickPos - lastTick_);
        if (n > kMaxBurst) n = kMaxBurst;
        lastTick_ = tickPos;
        return n;
    }

    void reset() { primed_ = false; }

private:
    static constexpr int kMaxBurst = 24;
    double lastTick_ = 0.0;
    bool primed_ = false;
};

class MidiClockChase {
public:
    double onClock(double nowMs) {
        if (lastMs_ >= 0.0) {
            const double dt = nowMs - lastMs_;
            if (dt > 60000.0 / (999.0 * 24.0) && dt < 60000.0 / (10.0 * 24.0)) {
                intervals_[write_++ % kWindow] = dt;
                if (count_ < kWindow) ++count_;
            } else {
                count_ = 0;
                write_ = 0;
            }
        }
        lastMs_ = nowMs;
        if (count_ < kWindow) return 0.0;
        double sum = 0.0;
        for (int i = 0; i < kWindow; ++i) sum += intervals_[i];
        return 60000.0 / (sum / kWindow * 24.0);
    }

    void reset() {
        lastMs_ = -1.0;
        count_ = 0;
        write_ = 0;
    }

private:
    static constexpr int kWindow = 24;
    double intervals_[kWindow] = {};
    double lastMs_ = -1.0;
    int count_ = 0;
    int write_ = 0;
};

}
