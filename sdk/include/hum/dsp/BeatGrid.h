#pragma once
#include <cmath>
#include <cstdint>

namespace hum {

struct BeatGrid {
    double  bpm = 120.0;
    int64_t offsetSamples = 0;

    double samplesPerBeat(double fileSampleRate) const {
        return (bpm > 0.0 ? 60.0 / bpm : 0.5) * fileSampleRate;
    }
    double beatToSample(double beat, double fileSampleRate) const {
        return (double) offsetSamples + beat * samplesPerBeat(fileSampleRate);
    }
    double sampleToBeat(double sample, double fileSampleRate) const {
        const double spb = samplesPerBeat(fileSampleRate);
        return spb > 0.0 ? (sample - (double) offsetSamples) / spb : 0.0;
    }
    double nearestBeatSample(double sample, double fileSampleRate) const {
        const double b = std::round(sampleToBeat(sample, fileSampleRate));
        return beatToSample(b, fileSampleRate);
    }
};

inline double syncRate(double gridBpm, double masterBpm) {
    return (gridBpm > 0.0 && masterBpm > 0.0) ? masterBpm / gridBpm : 1.0;
}

}
