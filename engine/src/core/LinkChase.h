#pragma once
#include <algorithm>
#include <cmath>

namespace hum {
namespace linkchase {

inline constexpr double kGain = 0.5;
inline constexpr double kMaxNudge = 0.01;
inline constexpr double kResyncBeats = 0.1;

inline double phaseError(double localBeats, double sessionPhase, double quantum) {
    if (quantum <= 0.0) return 0.0;
    double err = std::fmod(localBeats - sessionPhase, quantum);
    if (err > quantum * 0.5) err -= quantum;
    else if (err <= -quantum * 0.5) err += quantum;
    return err;
}

inline double chaseTempo(double sessionBpm, double errBeats) {
    const double nudge = std::clamp(kGain * errBeats, -kMaxNudge, kMaxNudge);
    return sessionBpm * (1.0 - nudge);
}

inline bool needsResync(double errBeats) {
    return std::abs(errBeats) > kResyncBeats;
}

struct Verdict {
    double tempo = 0.0;
    bool jump = false;
    double jumpToBeats = 0.0;
};

inline Verdict decide(double localBeats, double sessionBpm, double sessionPhase,
                      double quantum, bool playing, bool alignArmed) {
    Verdict v;
    if (!playing) { v.tempo = sessionBpm; return v; }
    const double err = phaseError(localBeats, sessionPhase, quantum);
    if (alignArmed || needsResync(err)) {
        v.tempo = sessionBpm;
        v.jump = true;
        v.jumpToBeats = std::max(0.0, localBeats - err);
        return v;
    }
    v.tempo = chaseTempo(sessionBpm, err);
    return v;
}

}
}
