#pragma once
#include <algorithm>
#include <cmath>

namespace hum {

inline constexpr double kPi = 3.14159265358979323846;
inline constexpr double kTwoPi = 6.28318530717958647692;
inline constexpr double kHalfPi = 1.57079632679489661923;
inline constexpr float kPiF = (float) kPi;
inline constexpr float kTwoPiF = (float) kTwoPi;
inline constexpr float kHalfPiF = (float) kHalfPi;
inline constexpr double kSqrtHalf = 0.70710678118654752440;
inline constexpr float kSqrtHalfF = (float) kSqrtHalf;

inline constexpr double kDefaultSampleRate = 44100.0;
inline constexpr int kSevenBitMax = 127;
inline constexpr int kMidiMax = kSevenBitMax;
inline constexpr float kMidiMaxF = (float) kMidiMax;
inline constexpr double kMidiMaxD = (double) kMidiMax;
inline constexpr double kSecondsPerMinute = 60.0;

inline constexpr double kA4Hz = 440.0;
inline constexpr double kA4Note = 69.0;
inline constexpr double kSemitonesPerOctave = 12.0;

inline double midiToHz(double note, double a4Hz = kA4Hz) {
    return a4Hz * std::pow(2.0, (note - kA4Note) / kSemitonesPerOctave);
}

inline double hzToMidi(double hz, double a4Hz = kA4Hz) {
    return hz > 0.0 && a4Hz > 0.0
               ? kA4Note + kSemitonesPerOctave * std::log2(hz / a4Hz)
               : kA4Note;
}

inline double dbToLin(double db) { return std::pow(10.0, db / 20.0); }
inline double linToDb(double x) { return 20.0 * std::log10(x + 1e-12); }

inline double smoothCoeff(double ms, double sr) {
    if (ms <= 0.0 || sr <= 0.0) return 0.0;
    return std::exp(-1.0 / (ms * 0.001 * sr));
}

inline double t60Feedback(double delaySeconds, double rt60Seconds) {
    if (rt60Seconds <= 0.0 || delaySeconds <= 0.0) return 0.0;
    return std::pow(10.0, -3.0 * delaySeconds / rt60Seconds);
}

inline float linkedPeak(const float* const* in, int numIn, int first, int count, int n) {
    float p = 0.0f;
    for (int c = first; c < first + count && c < numIn; ++c)
        if (in[c]) p = std::max(p, std::abs(in[c][n]));
    return p;
}

}
