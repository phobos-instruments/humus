#pragma once
#include <algorithm>
#include <cmath>

namespace hum {

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
