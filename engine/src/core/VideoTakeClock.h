#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum::videotake {

struct Advance {
    int hold = 0;
    bool write = false;
};

struct LapMark {
    double beat = 0.0;
    std::int64_t frame = 0;
};

class Clock {
public:
    explicit Clock(double fps) : fps_(fps > 0.0 ? fps : 30.0) {}

    Advance tick(double beat, double tempo, bool rolling) {
        if (!rolling) return {};
        if (startBeat_ < 0.0) {
            startBeat_ = beat;
            lastBeat_ = beat;
            written_ = 1;
            return {0, true};
        }
        if (beat < lastBeat_) laps_.push_back({beat, written_});
        else seconds_ += (beat - lastBeat_) * kSecondsPerMinute / (tempo > 0.0 ? tempo : 120.0);
        lastBeat_ = beat;
        const auto slot = (std::int64_t) std::llround(seconds_ * fps_);
        if (slot < written_) return {};
        Advance a;
        a.hold = (int) (slot - written_);
        a.write = true;
        written_ = slot + 1;
        return a;
    }

    bool started() const { return startBeat_ >= 0.0; }
    double startBeat() const { return startBeat_; }
    std::int64_t frames() const { return written_; }
    double fps() const { return fps_; }
    const std::vector<LapMark>& laps() const { return laps_; }

    std::int64_t samplesOf(std::int64_t frames, double sampleRate) const {
        return (std::int64_t) std::llround((double) frames * sampleRate / fps_);
    }

private:
    double fps_;
    double startBeat_ = -1.0;
    double lastBeat_ = 0.0;
    double seconds_ = 0.0;
    std::int64_t written_ = 0;
    std::vector<LapMark> laps_;
};

}
